#ifndef DCPLUSPLUS_CLIENT_DOWNLOAD_H_
#define DCPLUSPLUS_CLIENT_DOWNLOAD_H_

#include "forward.h"
#include "Transfer.h"
#include "MerkleTree.h"
#include "ZUtils.h"
#include "FilteredFile.h"
#include "Flags.h"

/**
 * Comes as an argument in the DownloadManagerListener functions.
 * Use it to retrieve information about the ongoing transfer.
 */
class Download : public Transfer, public Flags {
public:
	static const string ANTI_FRAG_EXT;

	enum {
		FLAG_USER_LIST = 0x01,
		FLAG_RESUME = 0x02,
		FLAG_ZDOWNLOAD = 0x04,
		FLAG_MULTI_CHUNK = 0x08,
		FLAG_ANTI_FRAG = 0x10,
		FLAG_TREE_DOWNLOAD = 0x20,
		FLAG_CHUNKED = 0x40,
		FLAG_TREE_TRIED = 0x80,
		FLAG_PARTIAL_LIST = 0x100,
		FLAG_TTH_CHECK = 0x200,
		FLAG_TESTSUR = 0x400,
		FLAG_CHECK_FILE_LIST = 0x800,
		FLAG_PARTIAL = 0x1000
	};

	Download(UserConnection& conn) throw();
	Download(UserConnection& conn, QueueItem& qi, bool partial) throw();

	void getParams(const UserConnection& aSource, StringMap& params);

	~Download();

	/** @return Target filename without path. */
	string getTargetFileName() const {
		return Util::getFileName(getTarget());
	}

	/** @internal */
	string getDownloadTarget() const {
		const string& tgt = (getTempTarget().empty() ? getTarget() : getTempTarget());
		return isSet(FLAG_ANTI_FRAG) ? tgt + ANTI_FRAG_EXT : tgt;
	}

	int64_t getChunkSize() const { return getSize() - getStartPos(); }
	
	/** @internal */
	TigerTree& getTigerTree() { return tt; }
	string& getPFS() { return pfs; }
	
	const TigerTree& getTigerTree() const { return tt; }
	const string& getPFS() const { return pfs; }

	/** @internal */
	AdcCommand getCommand(bool zlib) const;

	GETSET(string, target, Target);
	GETSET(string, tempTarget, TempTarget);

	uint32_t quickTick;

	GETSET(string*, source, Source);
	GETSET(OutputStream*, file, File);
	GETSET(bool, treeValid, TreeValid);

private:
	Download(const Download&);
	Download& operator=(const Download&);

	TigerTree tt;
	string pfs;
};


#endif /*DOWNLOAD_H_*/