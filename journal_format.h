#pragma once

// Taken from mongo/db/dur_journalformat.h

const unsigned Alignment = 8192;

#pragma pack(1)
/** beginning header for a journal/j._<n> file
    there is nothing important int this header at this time.  except perhaps version #.
*/
struct JHeader {

    char magic[2]; // "j\n". j means journal, then a linefeed, fwiw if you were to run "less" on the file or something...

    // x4142 is asci--readable if you look at the file with head/less -- thus the starting values were near
    // that.  simply incrementing the version # is safe on a fwd basis.
#if defined(_NOCOMPRESS)
    enum { CurrentVersion = 0x4148 };
#else
    enum { CurrentVersion = 0x4149 };
#endif
    unsigned short _version;

    // these are just for diagnostic ease (make header more useful as plain text)
    char n1;          // '\n'
    char ts[20];      // ascii timestamp of file generation.  for user reading, not used by code.
    char n2;          // '\n'
    char dbpath[128]; // path/filename of this file for human reading and diagnostics.  not used by code.
    char n3, n4;      // '\n', '\n'

    unsigned long long fileId; // unique identifier that will be in each JSectHeader. important as we recycle prealloced files

    char reserved3[8026]; // 8KB total for the file header
    char txt2[2];         // "\n\n" at the end

    bool versionOk() const { return _version == CurrentVersion; }
    bool valid() const { return magic[0] == 'j' && txt2[1] == '\n' && fileId; }
};

/** "Section" header.  A section corresponds to a group commit.
    len is length of the entire section including header and footer.
    header and footer are not compressed, just the stuff in between.
*/
struct JSectHeader {

    unsigned _sectionLen;          // unpadded length in bytes of the whole section

    unsigned long long seqNumber;  // sequence number that can be used on recovery to not do too much work
    unsigned long long fileId;     // matches JHeader::fileId
    unsigned sectionLen() const { return _sectionLen; }

    unsigned sectionLenWithPadding() const { 
        unsigned x = (sectionLen() + (Alignment-1)) & (~(Alignment-1));
        //dassert( x % Alignment == 0 );
        return x;
    }
};

/** an individual write operation within a group commit section.  Either the entire section should
    be applied, or nothing.  (We check the md5 for the whole section before doing anything on recovery.)
*/
struct JEntry {
    enum OpCodes {
        OpCode_Footer      = 0xffffffff,
        OpCode_DbContext   = 0xfffffffe,
        OpCode_FileCreated = 0xfffffffd,
        OpCode_DropDb      = 0xfffffffc,
        OpCode_Min         = 0xfffff000
    };
    union {
        unsigned len;    // length in bytes of the data of the JEntry. does not include the JEntry header
        OpCodes opcode;
    };

    unsigned ofs;  // offset in file

    // sentinel and masks for _fileNo
    enum {
        DotNsSuffix = 0x7fffffff, // ".ns" file
        LocalDbBit  = 0x80000000  // assuming "local" db instead of using the JDbContext
    };
    int _fileNo;   // high bit is set to indicate it should be the <dbpath>/local database
    // char data[len] follows

    bool isNsSuffix() const { return _fileNo == DotNsSuffix; }
    bool isLocalDbContext() const { return _fileNo & LocalDbBit; }

};

/** group commit section footer. md5 is a key field. */
struct JSectFooter {
    JSectFooter();
    JSectFooter(const void* begin, int len); // needs buffer to compute hash
    unsigned sentinel;
    unsigned char hash[16];
    unsigned long long reserved;
    char magic[4]; // "\n\n\n\n"

    /** used by recovery to see if buffer is valid
        @param begin the buffer
        @param len buffer len
        @return true if buffer looks valid
    */
    bool checkHash(const void* begin, int len) const;

    bool magicOk() const { return *((unsigned*)magic) == 0x0a0a0a0a; }
};

/** declares "the next entry(s) are for this database / file path prefix" */
struct JDbContext {
    JDbContext() : sentinel(JEntry::OpCode_DbContext) { }
    const unsigned sentinel;   // compare to JEntry::len -- zero is our sentinel
    //char dbname[];
};

struct FileCreatedOp {
    unsigned sentinel;
    unsigned long long reserved1;
    unsigned long long reserved2;
    unsigned long long len;  // size of file, not length of name
    char relativePath[1];    // 文件名的相对路径，以'\0'结尾的字符串
};

/** "last sequence number" */
struct LSNFile {
    unsigned ver;
    unsigned reserved2;
    unsigned long long lsn;
    unsigned long long checkbytes;
    unsigned long long reserved[8];

    void set(unsigned long long lsn);
    unsigned long long get();
};

#pragma pack()
