#pragma once

#pragma pack(1)

// Taken from mongo/db/structure/catalog/namespace.h

/**
  * This is used for storing a namespace on disk in a fixed witdh form
  * it should only be used for that, not for passing internally
  * for that, please use NamespaceString
  */
struct Namespace
{
    enum MaxNsLenValue {
        // Maximum possible length of name any namespace, including special ones like $extra.
        // This includes rum for the NUL byte so it can be used when sizing buffers.
        MaxNsLenWithNUL = 128,

        // MaxNsLenWithNUL excluding the NUL byte. Use this when comparing string lengths.
        MaxNsLen = MaxNsLenWithNUL - 1,

        // Maximum allowed length of fully qualified namespace name of any real collection.
        // Does not include NUL so it can be directly compared to string lengths.
        MaxNsColletionLen = MaxNsLen - 7/*strlen(".$extra")*/,
    };

    char buf[MaxNsLenWithNUL];
};

// Taken from mongo/db/diskloc.h

/** represents a disk location/offset on disk in a database.  64 bits.
  * it is assumed these will be passed around by value a lot so don't do anything to make them large
  * (such as adding a virtual function)
  */
struct DiskLoc
{
    int _a;     // this will be volume, file #, etc. but is a logical value could be anything depending on storage engine
    int ofs;

    bool isNull() const { return _a == -1; }

    enum SentinelValues {
        /* note NullOfs is different. todo clean up.  see refs to NullOfs in code - use is valid but outside DiskLoc context so confusing as-is. */
        NullOfs = -1,

        // Caps the number of files that may be allocated in a database, allowing about 32TB of
        // data per db.  Note that the DiskLoc and DiskLoc56Bit types supports more files than
        // this value, as does the data storage format.
        MaxFiles = 16000
    };
};

// Taken from mongo/db/structure/catalog/index_details.h

/* Details about a particular index. There is one of these effectively for each object in
       system.namespaces (although this also includes the head pointer, which is not in that
       collection).

       ** MemoryMapped Record ** (i.e., this is on disk data)
*/
class IndexDetails {
public:
    /**
        * btree head disk location
        * TODO We should make this variable private, since btree operations
        * may change its value and we don't want clients to rely on an old
        * value.  If we create a btree class, we can provide a btree object
        * to clients instead of 'head'.
        */
    DiskLoc head;

    /* Location of index info object. Format:

            { name:"nameofindex", ns:"parentnsname", key: {keypattobject}
            [, unique: <bool>, background: <bool>, v:<version>]
            }

        This object is in the system.indexes collection.  Note that since we
        have a pointer to the object here, the object in system.indexes MUST NEVER MOVE.
    */
    DiskLoc info;
};

// Taken from mongo/db/structure/catalog/namespace_details.h

/* NamespaceDetails : this is the "header" for a collection that has all its details.
       It's in the .ns file and this is a memory mapped region (thus the pack pragma above).
    */
const int Buckets = 19;
struct NamespaceDetails
{
    enum { NIndexesMax = 64, NIndexesExtra = 30, NIndexesBase  = 10 };

    /*-------- data fields, as present on disk : */

    DiskLoc _firstExtent;
    DiskLoc _lastExtent;

    /* NOTE: capped collections v1 override the meaning of deletedList.
                deletedList[0] points to a list of free records (DeletedRecord's) for all extents in
                the capped namespace.
                deletedList[1] points to the last record in the prev extent.  When the "current extent"
                changes, this value is updated.  !deletedList[1].isValid() when this value is not
                yet computed.
    */
    DiskLoc _deletedList[Buckets];

    // ofs 168 (8 byte aligned)
    struct Stats {
        // datasize and nrecords MUST Be adjacent code assumes!
        long long datasize; // this includes padding, but not record headers
        long long nrecords;
    } _stats;

    int _lastExtentSize;
    int _nIndexes;

    // ofs 192
    IndexDetails _indexes[NIndexesBase];

    // ofs 352 (16 byte aligned)
    int _isCapped;                         // there is wasted space here if I'm right (ERH)
    int _maxDocsInCapped;                  // max # of objects for a capped table, -1 for inf.

    double _paddingFactor;                 // 1.0 = no padding.
    // ofs 368 (16)
    int _systemFlags; // things that the system sets/cares about

    DiskLoc _capExtent; // the "current" extent we're writing too for a capped collection
    DiskLoc _capFirstNewRecord;

    unsigned short _dataFileVersion;       // NamespaceDetails version.  So we can do backward compatibility in the future. See filever.h
    unsigned short _indexFileVersion;
    unsigned long long _multiKeyIndexBits;

    // ofs 400 (16)
    unsigned long long _reservedA;
    long long _extraOffset;               // where the $extra info is located (bytes relative to this)

    int _indexBuildsInProgress;            // Number of indexes currently being built

    int _userFlags;
    char _reserved[72];
    /*-------- end data 496 bytes */
}; // NamespaceDetails

// Taken from mongo/db/structure/catalog/hashtab.h

struct Node
{
    int hash;
    Namespace k;
    NamespaceDetails value;

    bool inUse()
    {
        return hash != 0;
    }

    void setUnused()
    {
        hash = 0;
    }
};

// Taken from mongo/db/storage/extent.h

/* extents are datafile regions where all the records within the region
       belong to the same namespace.
  */
class Extent {
    public:
        enum { extentSignature = 0x41424344 };
        unsigned magic;
        DiskLoc myLoc;
        DiskLoc xnext, xprev; /* next/prev extent for this namespace */

        /* which namespace this extent is for.  this is just for troubleshooting really
           and won't even be correct if the collection were renamed!
        */
        Namespace nsDiagnostic;

        int length;   /* size of the extent, including these fields */
        DiskLoc firstRecord;
        DiskLoc lastRecord;
        char _extentData[4];

        static int HeaderSize() { return sizeof(Extent)-4; }
};

// Taken from mongo/db/storage/record.h

/* Record is a record in a datafile.  DeletedRecord is similar but for deleted space.
 */
class Record {
    public:
        enum HeaderSizeValue { HeaderSize = 16 };


        int netLength() const { return _lengthWithHeaders - HeaderSize; }

        int _lengthWithHeaders;
        int _extentOfs;
        int _nextOfs;
        int _prevOfs;

        /** be careful when referencing this that your write intent was correct */
        char _data[4];
};

#pragma pack()
