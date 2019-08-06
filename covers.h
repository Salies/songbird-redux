#ifndef __COVERS_H__
#define __COVERS_H__

#include <taglib/fileref.h>
#include <taglib/apefile.h>
#include <taglib/apetag.h>
#include <taglib/asffile.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/commentsframe.h>
#include <taglib/flacfile.h>
#include <taglib/id3v1genres.h>
#include <taglib/id3v2tag.h>
#include <taglib/mpcfile.h>
#include <taglib/mpegfile.h>
#include <taglib/mp4file.h>
#include <taglib/tag.h>
#include <taglib/taglib.h>
#include <taglib/textidentificationframe.h>
#include <taglib/tstring.h>
#include <taglib/vorbisfile.h>

/*class CCover
{
public:
	FUNCTIONS
};*/

bool getCover(const TagLib::FileRef& fr, sf::Texture& target);
bool drawCover(const TagLib::ByteVector& data, sf::Texture& texture);

#endif