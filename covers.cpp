/*The following code is an adaptation of the original Rainmeeter project one. It is shared under the same license.*/

/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include <SFML/Graphics.hpp>
#include "covers.h"

bool drawCover(const TagLib::ByteVector& data, sf::Texture& texture) {
	return texture.loadFromMemory(data.data(), data.size());
}

bool ExtractAPE(TagLib::APE::Tag* tag, sf::Texture& target)
{
	const TagLib::APE::ItemListMap& listMap = tag->itemListMap();
	if (listMap.contains("COVER ART (FRONT)"))
	{
		const TagLib::ByteVector nullStringTerminator(1, 0);
		TagLib::ByteVector item = listMap["COVER ART (FRONT)"].value();
		const int pos = item.find(nullStringTerminator);	// Skip the filename.
		if (pos != -1)
		{
			const TagLib::ByteVector& pic = item.mid(pos + 1);
			return drawCover(pic, target);
		}
	}

	return false;
}

bool ExtractID3(TagLib::ID3v2::Tag* tag, sf::Texture& target)
{
	const TagLib::ID3v2::FrameList& frameList = tag->frameList("APIC");
	if (!frameList.isEmpty())
	{
		// Just grab the first image.
		const auto* frame = (TagLib::ID3v2::AttachedPictureFrame*)frameList.front();
		return drawCover(frame->picture(), target);
	}

	return false;
}

bool ExtractFLAC(TagLib::FLAC::File* file, sf::Texture& target)
{
	const TagLib::List<TagLib::FLAC::Picture*>& picList = file->pictureList();
	if (!picList.isEmpty())
	{
		// Just grab the first image.
		const TagLib::FLAC::Picture* pic = picList[0];
		return drawCover(pic->data(), target);
	}

	return false;
}

bool ExtractASF(TagLib::ASF::File* file, sf::Texture& target)
{
	const TagLib::ASF::AttributeListMap& attrListMap = file->tag()->attributeListMap();
	if (attrListMap.contains("WM/Picture"))
	{
		const TagLib::ASF::AttributeList& attrList = attrListMap["WM/Picture"];
		if (!attrList.isEmpty())
		{
			// Let's grab the first cover. TODO: Check/loop for correct type.
			const TagLib::ASF::Picture& wmpic = attrList[0].toPicture();
			if (wmpic.isValid())
			{
				return drawCover(wmpic.picture(), target);
			}
		}
	}

	return false;
}

bool ExtractMP4(TagLib::MP4::File* file, sf::Texture& target)
{
	TagLib::MP4::Tag* tag = file->tag();
	const TagLib::MP4::ItemListMap& itemListMap = tag->itemListMap();
	if (itemListMap.contains("covr"))
	{
		const TagLib::MP4::CoverArtList& coverArtList = itemListMap["covr"].toCoverArtList();
		if (!coverArtList.isEmpty())
		{
			const TagLib::MP4::CoverArt* pic = &(coverArtList.front());
			return drawCover(pic->data(), target);
		}
	}

	return false;
}

bool getCover(const TagLib::FileRef& fr, sf::Texture& target) {
	bool found = false;

	if (TagLib::MPEG::File * file = dynamic_cast<TagLib::MPEG::File*>(fr.file()))
	{
		if (file->ID3v2Tag())
		{
			found = ExtractID3(file->ID3v2Tag(), target);
		}
		if (!found && file->APETag())
		{
			found = ExtractAPE(file->APETag(), target);
		}
	}
	else if (TagLib::FLAC::File * file = dynamic_cast<TagLib::FLAC::File*>(fr.file()))
	{
		found = ExtractFLAC(file, target);

		if (!found && file->ID3v2Tag())
		{
			found = ExtractID3(file->ID3v2Tag(), target);
		}
	}
	else if (TagLib::MP4::File * file = dynamic_cast<TagLib::MP4::File*>(fr.file()))
	{
		found = ExtractMP4(file, target);
	}
	else if (TagLib::ASF::File * file = dynamic_cast<TagLib::ASF::File*>(fr.file()))
	{
		found = ExtractASF(file, target);
	}
	else if (TagLib::APE::File * file = dynamic_cast<TagLib::APE::File*>(fr.file()))
	{
		if (file->APETag())
		{
			found = ExtractAPE(file->APETag(), target);
		}
	}
	else if (TagLib::MPC::File * file = dynamic_cast<TagLib::MPC::File*>(fr.file()))
	{
		if (file->APETag())
		{
			found = ExtractAPE(file->APETag(), target);
		}
	}
	else if(TagLib::RIFF::WAV::File * file = dynamic_cast<TagLib::RIFF::WAV::File*>(fr.file())){
		//this kind of file has ID3v2 tags https://taglib.org/api/classTagLib_1_1RIFF_1_1WAV_1_1File.html#a2bca63e227b0c2fa6cdd0c181360de96
		if (!found && file->ID3v2Tag())
		{
			found = ExtractID3(file->ID3v2Tag(), target);
		}
	}
	else if (TagLib::RIFF::AIFF::File * file = dynamic_cast<TagLib::RIFF::AIFF::File*>(fr.file())) {
		//this kind of file also has ID3v2 tags https://taglib.org/api/classTagLib_1_1RIFF_1_1AIFF_1_1File.html#add1a0d200c2356eb08c76057cdc54312
		//does this work or needs verification? note that ->tag() returns a pointer regardless of it having real tags or not
		if (!found && file->tag())
		{
			found = ExtractID3(file->tag(), target);
		}
	}

	return found;
}
