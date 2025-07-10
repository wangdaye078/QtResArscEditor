//********************************************************************
//	filename: 	F:\mygit\QtResArscEditor2\src\GuidFactory.h
//	desc:		
//
//	created:	wangdaye 22:6:2025   11:53
//********************************************************************
#ifndef GuidFactory_h__
#define GuidFactory_h__


class GuidFactory
{
public:
	GuidFactory();
	~GuidFactory();
	void reset();
	int getNewGuid();
private:
	int m_guid;
};

#endif // GuidFactory_h__
