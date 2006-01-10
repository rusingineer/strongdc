/* 
 * Copyright (C) 2001-2004 Jacek Sieka, j_s at telia com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "stdinc.h"
#include "DCPlusPlus.h"

#include "FinishedManager.h"
#include "FileChunksInfo.h"
#include "Client.h"

FinishedManager::~FinishedManager() throw() {
	Lock l(cs);
	for_each(downloads.begin(), downloads.end(), DeleteFunction());
	for_each(uploads.begin(), uploads.end(), DeleteFunction());
	for_each(MP3downloads.begin(), MP3downloads.end(), DeleteFunction());
	DownloadManager::getInstance()->removeListener(this);
	UploadManager::getInstance()->removeListener(this);

}

static int g_nMP3BitRate[6][16] = {
	{0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,-1},
	{0,32,48,56, 64, 80, 96,112,128,160,192,224,256,320,384,-1},
	{0,32,40,48, 56, 64, 80, 96,112,128,160,192,224,256,320,-1},
	{0,32,48,56, 64, 80, 96,112,128,144,160,176,192,224,256,-1},
	{0, 8,16,24, 32, 40, 48, 56, 64, 80, 96,112,128,144,160,-1},
	{0, 8,16,24, 32, 64, 80, 56, 64,128,160,112,128,256,320,-1},
};

enum ENMPEGVER
{
	MPEGVER_NA,		// reserved, N/A
	MPEGVER_25,		// 2.5
	MPEGVER_2,		// 2.0 (ISO/IEC 13818-3)
	MPEGVER_1		// 1.0 (ISO/IEC 11172-3)
};

// ENCHANNELMODE enumeration
enum ENCHANNELMODE
{
	MP3CM_STEREO,
	MP3CM_JOINT_STEREO,
	MP3CM_DUAL_CHANNEL,
	MP3CM_SINGLE_CHANNEL
};

struct MP3FRAMEHEADER
{
	unsigned emphasis : 2;			// M
	unsigned original : 1;			// L
	unsigned copyright : 1;			// K
	unsigned modeext : 2;			// J
	unsigned chanmode : 2;			// I
	unsigned privbit : 1;			// H
	unsigned padding : 1;			// G
	unsigned samplerate : 2;		// F
	unsigned bitrate : 4;			// E
	unsigned hascrc : 1;			// D
	unsigned mpeglayer : 2;			// C
	unsigned mpegver : 2;			// B
	unsigned framesync : 11;		// A
};

void ChangeEndian(void* pBuffer, int nBufSize)
{
	if (!pBuffer || !nBufSize)
		return;

	char temp;
	for (int i = 0; i < nBufSize / 2; i++)
	{
		temp = ((char*)pBuffer)[i];
		((char*)pBuffer)[i] = ((char*)pBuffer)[nBufSize - i - 1];
		((char*)pBuffer)[nBufSize - i - 1] = temp;
	}
}

	UINT m_nFrames;
	UINT m_nLength;					// in seconds
	ENMPEGVER m_enMPEGVersion;
	int m_nMPEGLayer;
	BOOL m_bHasCRC;
	int m_nBitRate;					// average if VBR, 0 if "free"
	int m_nSampleRate;
	string m_enChannelMode;
	string MPEGVer;

BOOL GetNextFrameHeader(HANDLE hFile, MP3FRAMEHEADER* pHeader, int nPassBytes)
{
	memset(pHeader,0,sizeof(*pHeader));
	if (nPassBytes > 0)
		SetFilePointer(hFile,nPassBytes,NULL,FILE_CURRENT);

	int n = 0;
	BOOL bReadOK;
	DWORD dwNumBytesRead;
	do
	{
		bReadOK = ReadFile(hFile,pHeader,4,&dwNumBytesRead,NULL);
		ChangeEndian(pHeader,4); // convert from big-endian to little-endian

		// only search in 10kb
		if (!bReadOK || dwNumBytesRead != 4 ||
			pHeader->framesync == 0x7FF || ++n > 10000)
			break;

		SetFilePointer(hFile,-3,NULL,FILE_CURRENT);
	}
	while (1);

	return (pHeader->framesync == 0x7FF);
}

void FinishedManager::remove(FinishedItem *item, bool upload /* = false */) {
	{
		Lock l(cs);
		FinishedItem::List *listptr = upload ? &uploads : &downloads;
		FinishedItem::Iter it = find(listptr->begin(), listptr->end(), item);

		if(it != listptr->end())
			listptr->erase(it);
		else
			return;
	}
	if (!upload)
		fire(FinishedManagerListener::RemovedDl(), item);
	else
		fire(FinishedManagerListener::RemovedUl(), item);
	delete item;		
}
	
void FinishedManager::removeAll(bool upload /* = false */) {
	{
		Lock l(cs);
		FinishedItem::List *listptr = upload ? &uploads : &downloads;
		for_each(listptr->begin(), listptr->end(), DeleteFunction());
		listptr->clear();
	}
	if (!upload)
		fire(FinishedManagerListener::RemovedAllDl());
	else
		fire(FinishedManagerListener::RemovedAllUl());
}

void FinishedManager::on(DownloadManagerListener::Complete, Download* d, bool) throw()
{
	if(!d->isSet(Download::FLAG_USER_LIST) && !SETTING(FINISHFILE).empty() && !BOOLSETTING(SOUNDS_DISABLED)) {
		PlaySound(Text::toT(SETTING(FINISHFILE)).c_str(), NULL, SND_FILENAME | SND_ASYNC);
	}
		
	if(d->isSet(Download::FLAG_MP3_INFO)) {

		m_nFrames = 0;
		m_nLength = 0;
		m_enMPEGVersion = MPEGVER_NA;
		m_nMPEGLayer = 0;
		m_nBitRate = 0;
		m_nSampleRate = 0;
		m_enChannelMode = MP3CM_STEREO;

		string strFile = d->getTarget();
		HANDLE hFile = NULL;
		if ((hFile = CreateFile(Text::utf8ToWide(strFile).c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL)) != INVALID_HANDLE_VALUE) {
			int nNextSearch = 0;

			MP3FRAMEHEADER sFrameHeader;
			memset(&sFrameHeader,0,sizeof(sFrameHeader));

			int nFrameBR = 0;
			double dLength = 0; // total length of file
			ULONG nTotalBR = 0; // total frames bit rate (used to calc. average)

			while (GetNextFrameHeader(hFile,&sFrameHeader,nNextSearch)) {
				if (m_nFrames < 1) {
					// first read the MPEG version
					switch (sFrameHeader.mpegver) {
						case 0: m_enMPEGVersion = MPEGVER_25; break;
						case 1: m_enMPEGVersion = MPEGVER_NA; break;
						case 2: m_enMPEGVersion = MPEGVER_2; break;
						case 3: m_enMPEGVersion = MPEGVER_1; break;
					}

					// next, read the MPEG layer description
					switch (sFrameHeader.mpeglayer) {
						case 0: m_nMPEGLayer = 0; break;
						case 1: m_nMPEGLayer = 3; break;
						case 2: m_nMPEGLayer = 2; break;
						case 3: m_nMPEGLayer = 1; break;
					}

					switch (m_enMPEGVersion) {
						case MPEGVER_NA: MPEGVer = "N/A"; break;
						case MPEGVER_25: MPEGVer = "MP3Pro2 - MPEG 2.5"; break;
						case MPEGVER_2: MPEGVer = "MP3Pro - MPEG 2"; break;
						case MPEGVER_1: MPEGVer = "MPEG 1"; break;
					}
					// read the bit for CRC or no CRC
					m_bHasCRC = sFrameHeader.hascrc;
				}

				// read the bitrate, based on the mpeg layer and version
				if (m_nMPEGLayer > 0) {
					if (m_enMPEGVersion == MPEGVER_1) {
						switch (m_nMPEGLayer) {
							case 1: nFrameBR = g_nMP3BitRate[0][sFrameHeader.bitrate]; break;
							case 2: nFrameBR = g_nMP3BitRate[1][sFrameHeader.bitrate]; break;
							case 3: nFrameBR = g_nMP3BitRate[2][sFrameHeader.bitrate]; break;
						}
					} else {
						switch (m_nMPEGLayer) {
							case 1: nFrameBR = g_nMP3BitRate[3][sFrameHeader.bitrate]; break;
							case 2: nFrameBR = g_nMP3BitRate[4][sFrameHeader.bitrate]; break;
							case 3: nFrameBR = g_nMP3BitRate[5][sFrameHeader.bitrate]; break;
						}
					}
				}

				// if nFrameBR is 0 or -1 then the bitrate is either free or bad
				if (nFrameBR > 0)
					nTotalBR += nFrameBR;
	
				// read sample rate
				if (m_enMPEGVersion == MPEGVER_1)
					switch (sFrameHeader.samplerate) {
						case 0: m_nSampleRate = 44100; break;
						case 1: m_nSampleRate = 48000; break;
						case 2: m_nSampleRate = 32000; break;
					}
				else if (m_enMPEGVersion == MPEGVER_2)
					switch (sFrameHeader.samplerate) {
						case 0: m_nSampleRate = 22050; break;
						case 1: m_nSampleRate = 24000; break;
						case 2: m_nSampleRate = 16000; break;
					}
				else if (m_enMPEGVersion == MPEGVER_25)
					switch (sFrameHeader.samplerate) {
						case 0: m_nSampleRate = 11025; break;
						case 1: m_nSampleRate = 12000; break;
						case 2: m_nSampleRate = 8000; break;
					}

				if (!m_nSampleRate)
					break;

				// read channel mode
				switch (sFrameHeader.chanmode) {
					case 0: m_enChannelMode = "Stereo"; break;
					case 1: m_enChannelMode = "Joint Stereo"; break;
					case 2: m_enChannelMode = "Dual Channel"; break;
					case 3: m_enChannelMode = "Single Channel"; break;
				}

				if (m_nMPEGLayer == 1)
					nNextSearch = (12000 * nFrameBR / m_nSampleRate + sFrameHeader.padding) * 4;
				else
					nNextSearch = 144000 * nFrameBR / m_nSampleRate + sFrameHeader.padding;

				nNextSearch -= 4; // the frame header was already read

				m_nFrames++;

				// calculate the length in seconds of this frame and add it to total
				if (nFrameBR)
					dLength += (double)(nNextSearch + 4) * 8 / (nFrameBR * 1000);
			}

			// if at least one frame was read, the MP3 is considered valid
			if (m_nFrames > 0) {
				m_nBitRate = nTotalBR / m_nFrames; // average the bitrate
			}

		}
		CloseHandle(hFile);
	
		FinishedMP3Item *item = new FinishedMP3Item(
			d->getTarget(), Util::toString(ClientManager::getInstance()->getNicks(d->getUserConnection()->getUser()->getCID())),
			Util::toString(ClientManager::getInstance()->getHubNames(d->getUserConnection()->getUser()->getCID())),
			d->getSize(),MPEGVer+" Verze "+Util::toString(m_nMPEGLayer), m_nSampleRate, m_nBitRate, m_enChannelMode,GET_TIME());

		Lock l(cs);
		MP3downloads.push_back(item);

		fire(FinishedManagerListener::Added_MP3Dl(), item);
		File::deleteFile(d->getTarget());

	} else {
		
		if(!d->isSet(Download::FLAG_TREE_DOWNLOAD) && (!d->isSet(Download::FLAG_USER_LIST) || BOOLSETTING(LOG_FILELIST_TRANSFERS))) {
			FinishedItem *item = new FinishedItem(
				d->getTarget(), Util::toString(ClientManager::getInstance()->getNicks(d->getUserConnection()->getUser()->getCID())),
				Util::toString(ClientManager::getInstance()->getHubNames(d->getUserConnection()->getUser()->getCID())),
				d->getSize(), d->getTotal(), (GET_TICK() - d->getStart()), GET_TIME(), d->isSet(Download::FLAG_CRC32_OK), d->getTTH() ? d->getTTH()->toBase32() : Util::emptyString);
			{
				Lock l(cs);
				downloads.push_back(item);
			}
			
			fire(FinishedManagerListener::AddedDl(), item);
		}
		char* buf = new char[STRING(FINISHED_DOWNLOAD).size() + MAX_PATH + 128];
		_snprintf(buf, STRING(FINISHED_DOWNLOAD).size() + MAX_PATH + 127, CSTRING(FINISHED_DOWNLOAD), d->getTargetFileName().c_str(), 
			d->getUserConnection()->getUser()->getFirstNick().c_str());
		buf[STRING(FINISHED_DOWNLOAD).size() + MAX_PATH + 127] = 0;
		LogManager::getInstance()->message(buf, true);
		delete[] buf;
	}
}

void FinishedManager::on(UploadManagerListener::Complete, Upload* u) throw()
{
	if(!u->isSet(Download::FLAG_USER_LIST))	{
		if ((!SETTING(UPLOADFILE).empty() && (!BOOLSETTING(SOUNDS_DISABLED))))
			PlaySound(Text::toT(SETTING(UPLOADFILE)).c_str(), NULL, SND_FILENAME | SND_ASYNC);
	}
	if(!u->isSet(Upload::FLAG_TTH_LEAVES) && (!u->isSet(Upload::FLAG_USER_LIST) || BOOLSETTING(LOG_FILELIST_TRANSFERS)) && u->getSize() == u->getFileSize()) {
		FinishedItem *item = new FinishedItem(
			u->getLocalFileName(), Util::toString(ClientManager::getInstance()->getNicks(u->getUserConnection()->getUser()->getCID())),
			Util::toString(ClientManager::getInstance()->getHubNames(u->getUserConnection()->getUser()->getCID())),
			u->getSize(), u->getTotal(), (GET_TICK() - u->getStart()), GET_TIME());
		{
			Lock l(cs);
			uploads.push_back(item);
		}

		fire(FinishedManagerListener::AddedUl(), item);
	}
}

string FinishedManager::getTarget(const string& aTTH){
	if(aTTH.empty()) return Util::emptyString;

	{
		Lock l(cs);

		for(FinishedItem::Iter i = downloads.begin(); i != downloads.end(); i++)
		{
			if((*i)->getTTH() == aTTH)
				return (*i)->getTarget();
		}
	}

	return Util::emptyString;
}

bool FinishedManager::handlePartialRequest(const TTHValue& tth, vector<u_int16_t>& outPartialInfo)
{

	string target = getTarget(tth.toBase32());

	if(target.empty()) return false;

	int64_t fileSize = File::getSize(target);

	if(fileSize < PARTIAL_SHARE_MIN_SIZE)
		return false;

	u_int16_t len = TigerTree::calcBlocks(fileSize);
	outPartialInfo.push_back(0);
	outPartialInfo.push_back(len);

	return true;
}

/**
 * @file
 * $Id$
 */
