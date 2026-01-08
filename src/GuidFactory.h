//********************************************************************
//	filename: 	F:\mygit\QtResArscEditor2\src\GuidFactory.h
//	desc:		
//
//	created:	wangdaye 22:6:2025   11:53
//********************************************************************
#ifndef GuidFactory_h__
#define GuidFactory_h__
#include <stdint.h>

class GuidFactory
{
public:
	GuidFactory();
	~GuidFactory();
	void reset();
	uint32_t getNewGuid();
private:
	uint32_t m_guid;
};

#endif // GuidFactory_h__
