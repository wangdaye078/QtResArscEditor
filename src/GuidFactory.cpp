#include "GuidFactory.h"

GuidFactory::GuidFactory()
	:m_guid(0)
{

}
GuidFactory::~GuidFactory()
{

}
void GuidFactory::reset()
{
	m_guid = 0;
}
uint32_t GuidFactory::getNewGuid()
{
	return m_guid++;
}
