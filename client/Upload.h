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
		FLAG_CHUNKED = 0x08
	};

	Upload(UserConnection& conn);
	~Upload();

	void getParams(const UserConnection& aSource, StringMap& params) const;

	GETSET(string, sourceFile, SourceFile);
	GETSET(InputStream*, stream, Stream);
};

#endif /*UPLOAD_H_*/
