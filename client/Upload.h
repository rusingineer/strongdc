#ifndef UPLOAD_H_
#define UPLOAD_H_

#include "forward.h"
#include "Transfer.h"
#include "Flags.h"

class Upload : public Transfer, public Flags {
public:
	enum Flags {
		FLAG_ZUPLOAD = 0x01,
		FLAG_PENDING_KICK = 0x02,
		FLAG_RESUMED = 0x04,
		FLAG_CHUNKED = 0x08,
		FLAG_PARTIAL = 0x10
	};

	Upload(UserConnection& conn, const string& path, const TTHValue& tth);
	~Upload();

	void getParams(const UserConnection& aSource, StringMap& params) const;

	GETSET(InputStream*, stream, Stream);
	GETSET(int64_t, fileSize, FileSize);
};

#endif /*UPLOAD_H_*/
