#include <stdio.h>
#include <string.h>
#include <string>
#include "mmap.h"
#include "util.h"
#include "journal_format.h"
#include "checksum.h"
#include "snappy/snappy.h"

struct JEntryInfo 
{
    int opCode;
    const char* p;
};

class JSectionIterator
{
public:
    JSectionIterator(const char* buf, int len) : buf_(buf), len_(len), offset_(0){}

    bool atEof() const
    {
        return offset_ >= len_;
    }

    bool next(JEntryInfo& info)
    {
        if (atEof())
            return false;

        JEntry* entry = (JEntry*)(buf_ + offset_);
        switch (entry->opcode)
        {
            case JEntry::OpCode_DbContext:
                info.opCode = JEntry::OpCode_DbContext;
                info.p = buf_ + offset_;
                offset_ += sizeof(entry->opcode);
                offset_ += strlen(buf_ + offset_) + 1;
                break;

            case JEntry::OpCode_FileCreated:
            {
                info.opCode = JEntry::OpCode_FileCreated;
                info.p = buf_ + offset_;
                FileCreatedOp* fileCreatedOp = (FileCreatedOp*)(buf_ + offset_);
                offset_ += sizeof(FileCreatedOp) + strlen(fileCreatedOp->relativePath);
                break;
            }

            case JEntry::OpCode_DropDb:
            case JEntry::OpCode_Footer:
                assert(false);
                break;

            default:
                info.opCode = 0;
                info.p = buf_ + offset_;
                offset_ += sizeof(JEntry) + entry->len;
                break;
        }

        return true;
    }

private:
    const char* buf_;
    int len_;
    int offset_;
};

class JSection
{
public:
    JSection(const char* buf) : buf_(buf), valid_(false)
    {
        Checksum checksum;
        checksum.gen(buf_, Header()->_sectionLen - sizeof(JSectFooter));
        if (memcmp(checksum.bytes, Footer()->hash, sizeof(checksum.bytes)) != 0)
            return;

        if (!Footer()->magicOk())
            return;

        if (!snappy::Uncompress(buf_ + sizeof(JSectHeader), Header()->_sectionLen - sizeof(JSectFooter) - sizeof(JSectHeader), &body_))
        {
            printf("snappy::Uncompress fail.\n");
            body_.clear();
            return;
        }

        valid_ = true;
    }

    const bool Valid() const
    {
        return valid_;
    }

    const JSectHeader* Header() const
    {
        return (JSectHeader*)buf_;
    }

    const JSectFooter* Footer() const
    {
        return (JSectFooter*)(buf_ + Header()->_sectionLen - sizeof(JSectFooter));
    }

    const std::string& Body() const
    {
        return body_;
    }

    unsigned lenWithPadding() const
    {
        return Header()->sectionLenWithPadding();
    }

    JSectionIterator iterator() const
    {
        return JSectionIterator(body_.data(), body_.size());
    }

private:
    const char* buf_;
    bool valid_;
    std::string body_;
};

#ifdef WIN32
#define FORMAT_64U "%I64u"
#else
#define FORMAT_64U "%llu"
#endif

static void printHeader(FILE* fp, const JHeader* header)
{
    fprintf(fp, "%c%c", header->magic[0], header->magic[1]);
    fprintf(fp, "%x", header->_version);
    fprintf(fp, "%c", header->n1);
    fprintf(fp, "%s", header->ts);
    fprintf(fp, "%c", header->n2);
    fprintf(fp, "%s", header->dbpath);
    fprintf(fp, "%c%c", header->n3, header->n4);
    fprintf(fp, FORMAT_64U, header->fileId);
    fprintf(fp, "%c%c", header->txt2[0], header->txt2[1]);
}

static void printSectionHeader(FILE* fp, const JSectHeader* sectHeader)
{
    fprintf(fp, "--------Section--------\n");
    fprintf(fp, "sectionLen = %d\n", sectHeader->_sectionLen);
    fprintf(fp, "seqNumber = "FORMAT_64U"\n", sectHeader->seqNumber);
    fprintf(fp, "fileId = "FORMAT_64U"\n", sectHeader->fileId);
}

//static void printSectionFooter(FILE* fp, const JSectFooter* sectFooter)
//{
//    fprintf(fp, "sentinel = %x\n", sectFooter->sentinel);
//}

static void printSectionEntry(FILE* fp, JEntryInfo info)
{
    switch (info.opCode)
    {
        case JEntry::OpCode_DbContext:
            fprintf(fp, "OpCode_DbContext\n");
            fprintf(fp, "%s\n", info.p + sizeof(JDbContext));
            break;

        case JEntry::OpCode_FileCreated:
        {
            fprintf(fp, "OpCode_FileCreated\n");
            FileCreatedOp* fileCreatedOp = (FileCreatedOp*)info.p;
            fprintf(fp, "len = "FORMAT_64U"\n", fileCreatedOp->len);
            fprintf(fp, "relative path = %s\n", fileCreatedOp->relativePath);
            break;
        }
            

        case JEntry::OpCode_DropDb:
        case JEntry::OpCode_Footer:
            assert(false);
            break;

        default:
        {
            fprintf(fp, "Op_Normal\n");
            JEntry* entry = (JEntry*)info.p;
            if (entry->isLocalDbContext())
                fprintf(fp, "local db\n");
            else if (entry->isNsSuffix())
                fprintf(fp, ".ns file\n");
            else
                fprintf(fp, "file no = %d\n", entry->_fileNo);

            fprintf(fp, "len = %d\n", entry->len);
            fprintf(fp, "ofs = %d\n", entry->ofs);
            break;
        }
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printf("Usage: ./print_journal journal_file\n");
        return -1;
    }

    unsigned long long fileSize = getFileSize(argv[1]);
    if (fileSize == 0)
    {
        printf("open journal file %s fail\n", argv[1]);
        return -1;
    }

    MemoryMappedFile mmfile(argv[1]);
    char* view = (char*)mmfile.map();
    if (view == NULL)
    {
        printf("open journal file %s fail\n", argv[1]);
        return -1;
    }

    FILE* fp = fopen("print_journal_out.txt", "w");

    JHeader* header = (JHeader*)view;
    if (!header->valid())
    {
        printf("invalid journal file %s\n", argv[1]);
        return -1;
    }
    printHeader(fp, header);

    unsigned long long offset = sizeof(JHeader);
    while (offset < fileSize)
    {
        JSection section(view + offset);
        if (!section.Valid())
        {
            printf("invalid journal file %s\n", argv[1]);
            return -1;
        }

        printSectionHeader(fp, section.Header());
        JSectionIterator it = section.iterator();
        JEntryInfo info;
        while (!it.atEof() && it.next(info))
        {
            printSectionEntry(fp, info);
        }
        offset += section.lenWithPadding();
    }

    fclose(fp);
    return 0;
}


