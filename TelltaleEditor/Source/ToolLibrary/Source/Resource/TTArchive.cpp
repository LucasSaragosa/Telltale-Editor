#include <Resource/TTArchive.hpp>
#include <Meta/Meta.hpp>
#include <Resource/Blowfish.hpp>
#include <Resource/Compression.hpp>

std::unique_ptr<Blowfish> TTArchive::_MakeBlowfish(U32 version) const
{
    if (_BlowfishKeyLen == 0)
        return nullptr;
    // version >= 7 uses the modified Blowfish variant.
    return std::unique_ptr<Blowfish>(
        TTE_NEW(Blowfish, MEMORY_TAG_TEMPORARY, version >= 7, _BlowfishKey, _BlowfishKeyLen));
}

// ===================================================================
// Entry table parsing
// ===================================================================

Bool TTArchive::_ParseEntries(DataStreamRef& stream, const DataStreamRef& fileData)
{
    U32 directoriesCount = 0;
    SerialiseDataU32(stream, 0, &directoriesCount, false);

    for (U32 i = 0; i < directoriesCount; i++)
    {
        I32 nameLength = 0;
        if (!stream->Read((U8*)&nameLength, sizeof(I32)))
            return false;

        if (nameLength < 1 || nameLength > 1023)
        {
            TTE_ASSERT(false, "Error: directory name length %d is too long", nameLength);
            return false;
        }

        U8 buf[1024];
        if (!stream->Read(buf, (U64)nameLength))
            return false;
        _Folders.push_back(String((const char*)buf, (size_t)nameLength));
    }

    U32 fileCount = 0;
    SerialiseDataU32(stream, 0, &fileCount, false);

    _Files.reserve(_Files.size() + fileCount);

    for (U32 i = 0; i < fileCount; i++)
    {
        I32 nameLength = 0;
        if (!stream->Read((U8*)&nameLength, sizeof(I32)))
            return false;

        if (nameLength < 1 || nameLength > 255)
        {
            TTE_ASSERT(false, "Error: file name length %d is too long", nameLength);
            return false;
        }

        U8 nameBuf[256];
        if (!stream->Read(nameBuf, (U64)nameLength))
            return false;
        String name((const char*)nameBuf, (size_t)nameLength);

        I32 folderIndex = 0;
        stream->Read((U8*)&folderIndex, sizeof(I32));
        TTE_ASSERT_AND_RETURN(folderIndex == 0, false,
            "Archive format needs checking (folder index != 0)");

        U32 offset = 0;
        U32 size = 0;
        SerialiseDataU32(stream, 0, &offset, false);
        SerialiseDataU32(stream, 0, &size, false);

        FileInfo inf{};
        inf.Name = std::move(name);
        inf.NameSymbol = Symbol(inf.Name);

        if (fileData)
        {
            inf.Stream = DataStreamManager::GetInstance()->CreateSubStream(
                fileData, (U64)offset, (U64)size);
            TTE_ATTACH_DBG_STR(inf.Stream.get(), "TTArchive SubStream: " + inf.Name);
        }

        _Files.push_back(std::move(inf));
    }

    std::sort(_Files.begin(), _Files.end(), FileInfoSorter{});
    return true;
}

// ===================================================================
// SerialiseIn
// ===================================================================

Bool TTArchive::SerialiseIn(DataStreamRef& in)
{
    TTE_ASSERT(in, "TTArchive::SerialiseIn: null input stream");

    U32 first = 0;
    SerialiseDataU32(in, 0, &first, false);

    // -----------------------------------------------------------------
    // Case 1: version == 0 -> plain legacy. `first` was the folder count.
    // -----------------------------------------------------------------
    if (first == 0)
    {
        _Version = 0;

        in->SetPosition(0);
        if (!_ParseEntries(in))
            return false;

        U32 filesOffset = 0;
        SerialiseDataU32(in, 0, &filesOffset, false);
        U32 filesSize = 0;
        SerialiseDataU32(in, 0, &filesSize, false); // unused

        DataStreamRef fileData = DataStreamManager::GetInstance()->CreateSubStream(
            in, (U64)filesOffset, in->GetSize() - (U64)filesOffset);

        // Rewind and re-parse now that we have fileData.
        _Files.clear();
        _Folders.clear();
        in->SetPosition(0);
        if (!_ParseEntries(in, fileData))
            return false;

        return true;
    }

    // -----------------------------------------------------------------
    // Case 2: possible versioned archive.
    // -----------------------------------------------------------------
    U32 second = 0;
    SerialiseDataU32(in, 0, &second, false);

    if (first >= 1 && first <= 9 && second <= 1)
    {
        const U32 version = first;
        _Version = version;

        std::unique_ptr<Blowfish> bf = _MakeBlowfish(version);

        const Bool isEncrypted = (second == 1);

        if (isEncrypted && !bf)
        {
            TTE_ASSERT(false, "TTArchive: header is encrypted but no Blowfish key was provided");
            return false;
        }

        if (version >= 2)
        {
            U32 unknown = 0;
            SerialiseDataU32(in, 0, &unknown, false);
        }

        U32 filesMode = 1;
        U32 chunkCount = 0;
        std::vector<U64> chunkSizes;
        U32 chunkSize = 0x10000;

        if (version >= 3)
        {
            SerialiseDataU32(in, 0, &filesMode, false);
            TTE_ASSERT_AND_RETURN(filesMode <= 2, false,
                "TTArchive: files mode %u not supported", filesMode);

            SerialiseDataU32(in, 0, &chunkCount, false);
            chunkSizes.resize(chunkCount);
            for (U32 i = 0; i < chunkCount; i++)
            {
                U32 sz = 0;
                SerialiseDataU32(in, 0, &sz, false);
                chunkSizes[i] = (U64)sz;
            }

            if (chunkCount > 0)
                chunkSize = 0x10000;

            U32 totalFileDataSize = 0;
            SerialiseDataU32(in, 0, &totalFileDataSize, false);
        }

        if (version >= 4)
        {
            U32 u1 = 0, u2 = 0;
            SerialiseDataU32(in, 0, &u1, false);
            SerialiseDataU32(in, 0, &u2, false);
        }

        if (version >= 5)
        {
            U32 xMode1 = 0, xMode2 = 0;
            SerialiseDataU32(in, 0, &xMode1, false);
            SerialiseDataU32(in, 0, &xMode2, false);
        }

        if (version >= 7)
        {
            U32 chunkSizeFactor = 0;
            SerialiseDataU32(in, 0, &chunkSizeFactor, false);
            chunkSize = chunkSizeFactor * 1024;
        }

        if (version >= 8)
        {
            U8 createSymbolTable = 0;
            in->Read(&createSymbolTable, 1);
            if (createSymbolTable > 0)
            {
                I32 unknown = 0;
                in->Read((U8*)&unknown, sizeof(I32));
            }
        }

        if (version >= 9)
        {
            U32 crc32 = 0;
            SerialiseDataU32(in, 0, &crc32, false);
        }

        I32 infoHeaderSize = 0;
        in->Read((U8*)&infoHeaderSize, sizeof(I32));
        TTE_ASSERT_AND_RETURN(infoHeaderSize > 0 && infoHeaderSize < 0x4000000, false,
            "TTArchive: invalid info header size %d", infoHeaderSize);

        U8* headerRaw = nullptr;
        U32 headerRawSize = (U32)infoHeaderSize;

        if (version >= 6 && filesMode == 2)
        {
            I32 compressedHeaderSize = 0;
            in->Read((U8*)&compressedHeaderSize, sizeof(I32));
            TTE_ASSERT_AND_RETURN(
                compressedHeaderSize > 0 && compressedHeaderSize < 0x4000000, false,
                "TTArchive: invalid compressed header size %d", compressedHeaderSize);

            U8* compressed = TTE_ALLOC((U64)compressedHeaderSize, MEMORY_TAG_TEMPORARY);
            if (!in->Read(compressed, (U64)compressedHeaderSize))
            {
                TTE_FREE(compressed);
                TTE_ASSERT_AND_RETURN(false, false, "TTArchive: failed to read compressed header");
            }

            Compression::Type mode = Compression::Detect(compressed, (U32)compressedHeaderSize);

            headerRaw = TTE_ALLOC((U64)headerRawSize, MEMORY_TAG_TEMPORARY);
            if (!Compression::Decompress(compressed, (U64)compressedHeaderSize,
                headerRaw, (U64)headerRawSize, mode))
            {
                TTE_FREE(compressed);
                TTE_FREE(headerRaw);
                TTE_ASSERT_AND_RETURN(false, false, "TTArchive: failed to decompress header");
            }

            TTE_FREE(compressed);
        }
        else
        {
            headerRaw = TTE_ALLOC((U64)headerRawSize, MEMORY_TAG_TEMPORARY);
            if (!in->Read(headerRaw, (U64)headerRawSize))
            {
                TTE_FREE(headerRaw);
                TTE_ASSERT_AND_RETURN(false, false, "TTArchive: failed to read header");
            }
        }

        if (isEncrypted)
        {
            if (!bf)
            {
                TTE_FREE(headerRaw);
                TTE_ASSERT_AND_RETURN(false, false,
                    "TTArchive: header is encrypted but no Blowfish available");
            }
            bf->Decrypt(headerRaw, headerRawSize);
        }

        DataStreamRef headerStream = DataStreamManager::GetInstance()->CreateBufferStream(
            "__Temp__ArchiveHeader", (U64)headerRawSize, headerRaw, true);

        U64 filesOffset = in->GetPosition();

        DataStreamRef fileData;
        if (chunkCount == 0 || filesMode != 2)
        {
            fileData = DataStreamManager::GetInstance()->CreateSubStream(
                in, filesOffset, in->GetSize() - filesOffset);
        }
        else
        {
            const U8* bfKey = isEncrypted ? _BlowfishKey : nullptr;
            U32 bfKeyLen = isEncrypted ? _BlowfishKeyLen : 0;

            if (isEncrypted && bfKeyLen == 0)
            {
                TTE_ASSERT_AND_RETURN(false, false,
                    "TTArchive: chunked file-data is encrypted but no Blowfish key was provided");
            }

            fileData = DataStreamManager::GetInstance()->CreateTTArchiveChunkedStream(
                in, filesOffset, chunkSize, chunkSizes,
                bfKey, bfKeyLen, _Version, isEncrypted);

            if (!fileData)
            {
                TTE_ASSERT_AND_RETURN(false, false,
                    "TTArchive: failed to create chunked data stream");
            }
        }

        if (!_ParseEntries(headerStream, fileData))
        {
            TTE_ASSERT_AND_RETURN(false, false, "TTArchive: failed to parse entry table");
        }

        return true;
    }

    // -----------------------------------------------------------------
    // Case 3: legacy. `first` was NOT a version number.
    // -----------------------------------------------------------------
    _Version = 0;

    in->SetPosition(0);

    if (second > 256)
    {
        std::unique_ptr<Blowfish> bf = _MakeBlowfish(0);
        if (!bf)
        {
            TTE_ASSERT(false, "TTArchive: legacy header is encrypted but no Blowfish key was provided");
            return false;
        }

        // ---- Encrypted legacy ----
        U32 headerSize = 0;
        SerialiseDataU32(in, 0, &headerSize, false);
        TTE_ASSERT_AND_RETURN(headerSize < 0x100000, false,
            "TTArchive: legacy header size too big");

        U8* headerRaw = TTE_ALLOC((U64)headerSize, MEMORY_TAG_TEMPORARY);
        if (!in->Read(headerRaw, (U64)headerSize))
        {
            TTE_FREE(headerRaw);
            TTE_ASSERT_AND_RETURN(false, false,
                "TTArchive: failed to read legacy encrypted header");
        }

        bf->Decrypt(headerRaw, headerSize);

        DataStreamRef headerStream = DataStreamManager::GetInstance()->CreateBufferStream(
            "__Temp__ArchiveHeader", (U64)headerSize, headerRaw, true);

        U64 filesOffset = in->GetPosition();

        DataStreamRef fileData = DataStreamManager::GetInstance()->CreateSubStream(
            in, filesOffset, in->GetSize() - filesOffset);

        if (!_ParseEntries(headerStream, fileData))
        {
            TTE_ASSERT_AND_RETURN(false, false,
                "TTArchive: failed to parse legacy encrypted header");
        }

        return true;
    }
    else
    {
        // ---- Plain legacy ----
        in->SetPosition(0);
        if (!_ParseEntries(in))
            return false;

        U32 filesOffset = 0;
        SerialiseDataU32(in, 0, &filesOffset, false);
        U32 filesSize = 0;
        SerialiseDataU32(in, 0, &filesSize, false); // unused

        DataStreamRef fileData = DataStreamManager::GetInstance()->CreateSubStream(
            in, (U64)filesOffset, in->GetSize() - (U64)filesOffset);

        _Files.clear();
        _Folders.clear();
        in->SetPosition(0);
        if (!_ParseEntries(in, fileData))
            return false;

        return true;
    }
}

Bool TTArchive::SerialiseOut(DataStreamRef& in)
{
    TTE_ASSERT("IMPLEMENT ME");
    return false; // TODO
}