// No license, No copyright... use it if you want ;-)

#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"
#include "UserInfo.h"



UserInfo::UserInfo(const User::Ptr& u, const UserListColumns* pListColumns) 
	: UserInfoBase(u), op(false) { 
	m_pListColumns = pListColumns;
	update();
};

const string& UserInfo::getText(int col) const {
	int nHardCol = col;
	if (m_pListColumns)
		nHardCol = m_pListColumns->RemapListColumnToDataColumn(col);
	switch(nHardCol) {
		case COLUMN_NICK: return user->getNick();
		case COLUMN_SHARED: return shared;
		case  COLUMN_EXACT_SHARED: return exactshare;
		case COLUMN_DESCRIPTION: return user->getDescription();
		case COLUMN_TAG: return user->getTag();
		case COLUMN_CONNECTION: return user->getConnection();
		case COLUMN_UPLOAD_SPEED: return uuploadSpeed;
		case COLUMN_EMAIL: return user->getEmail();
		case COLUMN_CLIENTID: return user->getClientType();
		case  COLUMN_VERSION: return user->getVersion();
		case  COLUMN_MODE: return user->getMode();
		case  COLUMN_HUBS: return user->getHubs();
		case  COLUMN_SLOTS: return user->getSlots();
		case  COLUMN_ISP: return user->getHost();		
		case  COLUMN_IP: return user->getIp();
		case  COLUMN_PK: return user->getPk();
		case  COLUMN_LOCK: return user->getLock();
		case  COLUMN_SUPPORTS: return user->getSupports();

		default: return Util::emptyString;
	}
}

int UserInfo::compareItems(const UserInfo* a, const UserInfo* b, int col)  {
	if(a == NULL || b == NULL){
		dcdebug("UserInfo::compareItems: pointer == NULL\n");
		return 0;
	}

	int nHardCol = col;
	if (a->m_pListColumns)
		nHardCol = a->m_pListColumns->RemapListColumnToDataColumn(col);
	switch(nHardCol) {
		case COLUMN_NICK:
			if(a->getOp() && !b->getOp()) {
				return -1;
			} else if(!a->getOp() && b->getOp()) {
				return 1;
			}
			return Util::stricmp(a->user->getNick(), b->user->getNick());	
		case COLUMN_SHARED:	return compare(a->user->getBytesShared(), b->user->getBytesShared());
		case COLUMN_EXACT_SHARED: return compare(a->user->getBytesShared(), b->user->getBytesShared());
		case COLUMN_DESCRIPTION: return Util::stricmp(a->user->getDescription(), b->user->getDescription());
		case COLUMN_TAG: return Util::stricmp(a->user->getTag(), b->user->getTag());
		case COLUMN_CONNECTION: return Util::stricmp(a->user->getConnection(), b->user->getConnection());
		case COLUMN_UPLOAD_SPEED: return compare(a->uuploadSpeed,b->uuploadSpeed);
		case COLUMN_EMAIL: return Util::stricmp(a->user->getEmail(), b->user->getEmail());
		case COLUMN_CLIENTID: return Util::stricmp(a->user->getClientType(), b->user->getClientType());
		case COLUMN_VERSION: return Util::stricmp(a->user->getVersion(), b->user->getVersion());
		case COLUMN_MODE: return Util::stricmp(a->user->getMode(), b->user->getMode());
		case COLUMN_HUBS: return compare(Util::toInt(a->user->getHubs()), Util::toInt(b->user->getHubs()));
		case COLUMN_SLOTS: return compare(Util::toInt(a->user->getSlots()), Util::toInt(b->user->getSlots()));
		case COLUMN_IP: return compare(Util::toInt(a->user->getIp()), Util::toInt(b->user->getIp()));
		case COLUMN_ISP: return compare(a->user->getHost(),b->user->getHost());
		case COLUMN_PK: return  compare(a->user->getPk(),b->user->getPk()); ;
		case COLUMN_LOCK: return  compare(a->user->getLock(),b->user->getLock()); ;
		case COLUMN_SUPPORTS: return compare(a->user->getSupports(),b->user->getSupports()); 
		default: return 0;
	}
}

void UserInfo::update() {
	if(user->getDownloadSpeed()<1)
		{
			string s1=user->getDescription();
			if((s1.find_last_of('@')==-1)){
			const string& tmp = user->getConnection();
			int status = user->getStatus();
			string Omezeni = user->getUpload();

			if (!Omezeni.empty())
			 { s1=Util::formatBytes(Util::toInt64(Omezeni)*1024)+"/s";}
			else
			if( (status == 8) || (status == 9)  || (status == 10) || (status == 11)) { s1 = ">=100 kB/s"; }
			else if(tmp == "28.8Kbps") { s1 = "*max. 2.1 kB/s"; }
			else if(tmp == "33.6Kbps") { s1 = "*max. 3 kB/s"; }
			else if(tmp == "56Kbps") { s1 = "*max. 4.2 kB/s"; }
			else if(tmp == SettingsManager::connectionSpeeds[SettingsManager::SPEED_MODEM]) { s1 = "*max. 6 kB/s"; }
			else if(tmp == SettingsManager::connectionSpeeds[SettingsManager::SPEED_ISDN]) { s1 = "*max. 10 kB/s"; }
			else { s1 = "N/A"; }
					}
				else 
			{
				s1.erase(0,s1.find_last_of('@')+1);
			}
			uuploadSpeed = s1;
		} else uuploadSpeed = Util::formatBytes(user->getDownloadSpeed())+"/s";

	shared = Util::formatBytes(user->getBytesShared());
	op = user->isSet(User::OP);
	exactshare = Util::formatNumber(user->getBytesShared());
	
}

static int def_columnSizes[] = { 100, 75, 75, 75, 100, 75, 40, 100, 40, 40, 40, 40, 40, 100, 100, 100, 100, 175 };
static int def_columnIndexes[] = { 
	UserInfo::COLUMN_NICK, UserInfo::COLUMN_SHARED, UserInfo::COLUMN_EXACT_SHARED, 
	UserInfo::COLUMN_DESCRIPTION, UserInfo::COLUMN_TAG, UserInfo::COLUMN_CONNECTION,
	UserInfo::COLUMN_UPLOAD_SPEED,
	UserInfo::COLUMN_EMAIL, UserInfo::COLUMN_CLIENTID, UserInfo::COLUMN_VERSION, 
	UserInfo::COLUMN_MODE, UserInfo::COLUMN_HUBS, UserInfo::COLUMN_SLOTS, UserInfo::COLUMN_ISP,
	UserInfo::COLUMN_IP, UserInfo::COLUMN_PK, UserInfo::COLUMN_LOCK, UserInfo::COLUMN_SUPPORTS};

static int def_ColumnShowSettings[] = {
	0, SettingsManager::SHOW_SHARED, SettingsManager::SHOW_EXACT_SHARED, SettingsManager::SHOW_DESCRIPTION, 
	SettingsManager::SHOW_TAG, SettingsManager::SHOW_CONNECTION, SettingsManager::SHOW_UPLOAD, SettingsManager::SHOW_EMAIL, 
	SettingsManager::SHOW_CLIENT, SettingsManager::SHOW_VERSION, SettingsManager::SHOW_MODE, 
	SettingsManager::SHOW_HUBS, SettingsManager::SHOW_SLOTS, SettingsManager::SHOW_ISP ,SettingsManager::SHOW_IP,
	SettingsManager::SHOW_PK, SettingsManager::SHOW_LOCK, SettingsManager::SHOW_SUPPORTS};

/*static */ResourceManager::Strings UserListColumns::def_columnNames[] = { ResourceManager::NICK, ResourceManager::SHARED, ResourceManager::EXACT_SHARED, 
ResourceManager::DESCRIPTION, ResourceManager::TAG, ResourceManager::CONNECTION, ResourceManager::AVERAGE_UPLOAD, ResourceManager::EMAIL, 
ResourceManager::CLIENTID, ResourceManager::VERSION, ResourceManager::MODE, ResourceManager::HUBS, ResourceManager::SLOTS, ResourceManager::ISP,
ResourceManager::SETTINGS_IP, ResourceManager::PK, ResourceManager::LOCK,  ResourceManager::SUPPORTS };

//ResourceManager::Strings UserListColumns::columnNames[UserInfo::COLUMN_LAST];

UserListColumns::UserListColumns() {
	memcpy(m_nColumnSizes, def_columnSizes, sizeof(int)* UserInfo::COLUMN_LAST);
	memcpy(m_nColumnIndexes, def_columnIndexes, sizeof(int)* UserInfo::COLUMN_LAST);
	memset(m_bColumnUsing,1, sizeof(bool)* UserInfo::COLUMN_LAST);
	memset(m_nColumnIdxData,-1, sizeof(int)* UserInfo::COLUMN_LAST);
	
	
}

void UserListColumns::ReadFromSetup() {
	/*for(int j=0; j<UserInfo::COLUMN_LAST; j++) {
		columnNames[j] = def_columnNames[j];
	}*/
	WinUtil::splitTokens(m_nColumnIndexes, SETTING(HUBFRAME_ORDER), UserInfo::COLUMN_LAST);
	WinUtil::splitTokens(m_nColumnSizes, SETTING(HUBFRAME_WIDTHS), UserInfo::COLUMN_LAST);

	// Nacteni pouzivanych sloupcu - zapneme vse
	memset(m_bColumnUsing,1, sizeof(bool)* UserInfo::COLUMN_LAST);
	// a ted ze setupu
	for(int cnt = 0; cnt < UserInfo::COLUMN_LAST; cnt++) {
		if (def_ColumnShowSettings[cnt] == 0) // Nema setup
			continue;
		m_bColumnUsing[cnt] = SettingsManager::getInstance()->getBool((SettingsManager::IntSetting)def_ColumnShowSettings[cnt], true);
	}


	// Osetreni nulovych sloupcu po nacteni ze settings ktere jsou nyni zaple
	for(int cnt = 0; cnt < UserInfo::COLUMN_LAST; cnt++) {
		if ((m_nColumnSizes[cnt] <= 0)&&(m_bColumnUsing[cnt]))
			m_nColumnSizes[cnt] = def_columnSizes[cnt];
	}

	RecalcIdxData();
}

void UserListColumns::WriteToSetup(TypedListViewCtrl<UserInfo, IDC_USERS>& UserList) {
	WinUtil::saveHeaderOrder(UserList, SettingsManager::HUBFRAME_ORDER, 
			SettingsManager::HUBFRAME_WIDTHS, UserInfo::COLUMN_LAST, m_nColumnIndexes, m_nColumnSizes);

}

void UserListColumns::RecalcIdxData() {
	// Nastaveni indexu pro vraceni textu datovych sloupcu
	int nDataIdx = 0;
	for(int cnt = 0; cnt < UserInfo::COLUMN_LAST; cnt++) {
		if (!m_bColumnUsing[cnt]) {
			m_nColumnIdxData[cnt] = -1;
			continue;
		}
		m_nColumnIdxData[cnt] = nDataIdx;
		nDataIdx++;
	}
}

int UserListColumns::RemapDataColumnToListColumn(int nDataCol) const {
	if ((nDataCol < 0)||(nDataCol >= UserInfo::COLUMN_LAST))
		return -1;
	return m_nColumnIdxData[nDataCol];
}

int UserListColumns::RemapListColumnToDataColumn(int nDataCol) const {
	if ((nDataCol < 0)||(nDataCol >= UserInfo::COLUMN_LAST))
		return -1;
	for(int cnt = 0; cnt < UserInfo::COLUMN_LAST; cnt++) {
		if (m_nColumnIdxData[cnt] == nDataCol)
			return cnt;
	}
	return -1;
}

bool UserListColumns::IsColumnUsed(int nHardColumn) const {
	if ((nHardColumn < 0)||(nHardColumn >= UserInfo::COLUMN_LAST))
		return false;
	return m_bColumnUsing[nHardColumn];

}

void UserListColumns::SetToList(TypedListViewCtrl<UserInfo, IDC_USERS>& UserList) {
	for(int j=0; j<UserInfo::COLUMN_LAST; j++) {
		if (!IsColumnUsed(j))
			continue;
		int fmt = ((j == UserInfo::COLUMN_SHARED) || (j == UserInfo::COLUMN_EXACT_SHARED) || (j == UserInfo::COLUMN_HUBS) || 
			(j == UserInfo::COLUMN_SLOTS) || (j == UserInfo::COLUMN_UPLOAD_SPEED)) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		UserList.InsertColumn(j, CSTRING_I(def_columnNames[j]), fmt, m_nColumnSizes[j], j);
	}
}

void UserListColumns::SwitchColumnVisibility(int nHardColumn, TypedListViewCtrl<UserInfo, IDC_USERS>& UserList) {
	if ((nHardColumn < 0)||(nHardColumn >= UserInfo::COLUMN_LAST))
		return;
	bool bColumnIsOn = m_bColumnUsing[nHardColumn];
	bColumnIsOn = !bColumnIsOn;
	SetColumnVisibility(nHardColumn, UserList, bColumnIsOn);
}

void UserListColumns::SetColumnVisibility(int nHardColumn, TypedListViewCtrl<UserInfo, IDC_USERS>& UserList, bool bColumnIsOn) {
	if ((nHardColumn < 0)||(nHardColumn >= UserInfo::COLUMN_LAST))
		return;
	if (m_bColumnUsing[nHardColumn] == bColumnIsOn)
		return;
	if (m_bColumnUsing[nHardColumn]) {
		// Sloupec rusime
		int nListColumn = RemapDataColumnToListColumn(nHardColumn);
		UserList.DeleteColumn(nListColumn);
		m_bColumnUsing[nHardColumn] = bColumnIsOn;
		RecalcIdxData();
	} else {
		// Sloupec pridavame
		m_bColumnUsing[nHardColumn] = bColumnIsOn;
		RecalcIdxData();
		int nListColumn = RemapDataColumnToListColumn(nHardColumn);
		if (m_nColumnSizes[nHardColumn] == 0)
			m_nColumnSizes[nHardColumn] = def_columnSizes[nHardColumn];
		int fmt = ((nListColumn == UserInfo::COLUMN_SHARED) || (nListColumn == UserInfo::COLUMN_EXACT_SHARED) || (nListColumn == UserInfo::COLUMN_HUBS) ||
			 (nListColumn == UserInfo::COLUMN_SLOTS) || (nListColumn == UserInfo::COLUMN_UPLOAD_SPEED)) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		UserList.InsertColumn(nListColumn, CSTRING_I(def_columnNames[nHardColumn]), fmt, m_nColumnSizes[nHardColumn], nListColumn);
	}
	if (def_ColumnShowSettings[nHardColumn] != 0) { 
		// Ma setup, tak sup s nastavenim do setupu
//		SettingsManager::getInstance()->set((SettingsManager::IntSetting)def_ColumnShowSettings[nHardColumn], bColumnIsOn);
	}
}
