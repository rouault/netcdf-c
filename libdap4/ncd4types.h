/*********************************************************************
 *   Copyright 2016, UCAR/Unidata
 *   See netcdf/COPYRIGHT file for copying and redistribution conditions.
 *********************************************************************/

/*
Type declarations and associated constants
are defined here.	
*/

#ifndef D4TYPES_H
#define D4TYPES_H 1

#undef COMPILEBYDEFAULT

#define long64 long long
#define ncerror int

/* Misc. code controls */
#define FILLCONSTRAINT TRUE

#define DEFAULTSTRINGLENGTH 64

/* Size of the checksum */
#define CHECKSUMSIZE 4

/**************************************************/
/* sigh, do the forwards */

typedef struct NCD4INFO NCD4INFO;
typedef enum NCD4CSUM NCD4CSUM;
typedef enum NCD4mode NCD4mode;
typedef enum NCD4translation NCD4translation;
typedef struct NCD4curl NCD4curl;
typedef struct NCD4meta NCD4meta;
typedef struct NCD4globalstate NCD4globalstate;
typedef struct NCD4node NCD4node;
typedef struct NCD4params NCD4params;

/**************************************************/
/* DMR Tree node sorts */

/* Concepts from netcdf-4 data model */
/* Define as powers of 2 so we can create a set */
typedef enum NCD4sort {
    NCD4_NULL=0,
    NCD4_ATTR=1,
    NCD4_ATTRSET=2,
    NCD4_XML=4, /* OtherXML */
    NCD4_DIM=8,
    NCD4_GROUP=16, /* Including Dataset */
    NCD4_TYPE=32, /* atom types, opaque, enum, struct or seq */
    NCD4_VAR=64, /* Variable or field */
    NCD4_ECONST=128,
} NCD4sort;

#define ISA(sort,flags) ((sort) & (flags))

#define ISATTR(sort) ISA((sort),(NCD4_ATTR))
#define ISDIM(sort) ISA((sort),(NCD4_DIM))
#define ISGROUP(sort) ISA((sort),(NCD4_GROUP))
#define ISTYPE(sort) ISA((sort),(NCD4_TYPE))
#define ISVAR(sort) ISA((sort),(NCD4_VAR))
#define ISECONST(sort) ISA((sort),(NCD4_ECONST))

/* Define some nc_type aliases */
#define NC_NULL NC_NAT
#define NC_SEQ NC_VLEN
#define NC_STRUCT NC_COMPOUND

#define ISCMPD(subsort) ((subsort) == NC_SEQ || (subsort) == NC_STRUCT)

/**************************************************/
/* Special attributes; When defining netcdf-4,
   these are suppressed, except for UCARTAGMAPS
*/

#define RESERVECHAR '_'

#define UCARTAG        "_edu.ucar."
#define UCARTAGVLEN     "_edu.ucar.isvlen"
#define UCARTAGOPAQUE   "_edu.ucar.opaque.size"
#define UCARTAGORIGTYPE "_edu.ucar.orig.type"

/* These are attributes inserted into the netcdf-4 file */
#define NC4TAGMAPS      "_edu.ucar.maps"

/**************************************************/
/* Misc.*/

/* Define possible translation modes */
enum NCD4translation {
NCD4_NOTRANS = 0, /* Apply straight DAP4->NetCDF4 translation */
NCD4_TRANSNC4 = 1, /* Use _edu.ucar flags to achieve better translation */
};

/* Define possible debug flags */
#define NCF_DEBUG_NONE  0
#define NCF_DEBUG_COPY  1 /* Dump data into the substrate and close it rather than abortiing it */

/* Define possible retrieval modes */
enum NCD4mode {
NCD4_DMR = 1,
NCD4_DAP = 2,
};


/* Define possible checksum modes */
enum NCD4CSUM {
    NCD4_CSUM_NONE   = 0,
    NCD4_CSUM_IGNORE = 1, /*=> checksums are present; do not validate */
    NCD4_CSUM_DMR    = 2, /*=> compute checksums for DMR requests only*/
    NCD4_CSUM_DAP    = 4, /*=> compute checksums for DAP requests only*/
    NCD4_CSUM_ALL    = 6  /*=> compute checksums for both kinds of requests */
};

/* Define storage for all the primitive types (plus vlen) */
union ATOMICS {
    char i8[8];
    unsigned char u8[8];
    short i16[4];
    unsigned short u16[4];
    int i32[2];
    unsigned int u32[2];
    long long i64[1];
    unsigned long long u64[1];
    float f32[2];
    double f64[1];
#if SIZEOF_VOIDP == 4
    char* s[2];
#elif SIZEOF_VOIDP == 8
    char* s[1];
#endif
#if (SIZEOF_VOIDP + SIZEOF_SIZE_T) == 8
    nc_vlen_t vl[1];
#endif
};

/**************************************************/
/* !Node type for the NetCDF-4 metadata produced from
   parsing the DMR tree.
   We only use a single node type tagged with the sort.
   Some information is not kept (e.g. attributes).
*/
struct NCD4node {
    NCD4sort sort; /* gross discriminator */
    nc_type subsort; /* subcases of sort */
    char* name; /* Raw unescaped */
    NCD4node*  container; /* parent object: e.g. group, enum, compound...*/
    int visited; /* for recursive walks of all nodes */
    /* Sort specific fields */
    NClist* groups;   /* NClist<NCD4node*> groups in group */
    NClist* vars;   /* NClist<NCD4node*> vars in group, fields in struct/seq */
    NClist* types;   /* NClist<NCD4node*> types in group */
    NClist* dims;    /* NClist<NCD4node*>; dimdefs in group, dimrefs in vars */
    NClist* attributes; /* NClist<NCD4node*> */
    NClist* maps;       /* NClist<NCD4node*> */
    NClist* xmlattributes; /* NClist<String> */
    NCD4node* basetype;
    struct { /* sort == NCD4_ATTRIBUTE */
        NClist* values;
    } attr;
    struct { /* sort == NCD4_OPAQUE */
	long long size; /* 0 => var length */
    } opaque;
    struct { /* sort == NCD4_DIMENSION */
	long long size;
	int isunlimited;
	int isanonymous;
    } dim;
    struct { /* sort == NCD4_ECONST || sort == NCD4_ENUM */    
        union ATOMICS ecvalue;
        NClist* econsts; /* NClist<NCD4node*> */
    } en;
    struct { /* sort == NCD4_GROUP */
	NClist* elements;   /* NClist<NCD4node*> everything at the top level in a group */
        int isdataset;
        char* dapversion;
        char* dmrversion;
        char* datasetname;
        NClist* varbyid; /* NClist<NCD4node*> indexed by varid */
    } group;
    struct { /* Meta Info */
        int id; /* Relevant netcdf id; interpretation depends on sort; */
	int isfixedsize; /* sort == NCD4_TYPE; Is this a fixed size (recursively) type? */
	d4size_t dapsize; /* size of the type as stored in the dap data; will, as a rule,
                             be same as memsize only for types <= NC_UINT64 */
        nc_type cmpdid; /*netcdf id for the compound type created for seq type */
	size_t memsize; /* size of a memory instance without taking dimproduct into account,
                           but taking compound alignment into account  */
        d4size_t offset; /* computed structure field offset in memory */
        size_t alignment; /* computed structure field alignment in memory */
    } meta;
    struct { /* Data compilation info */
        int flags; /* See d4data for actual flags */
	D4blob dap4data; /* offset and start pos for this var's data in serialization */
        unsigned int remotechecksum; /* toplevel variable checksum as sent by server*/    
        unsigned int localchecksum; /* toplevel variable checksum as computed by client */    
    } data;
    struct { /* Track netcdf-4 conversion info */
	int isvlen;	/*  _edu.ucar.isvlen */
	/* Split UCARTAGORIGTYPE into group plus name */
	struct {
  	    NCD4node* group;
	    char* name;
	} orig;
	/* Represented elsewhare: _edu.ucar.opaque.size */ 
    } nc4;
};

/* Tracking info about the serialized input before and after de-chunking */
typedef struct NCD4serial {
    size_t rawsize; /* |rawdata| */ 
    void* rawdata;
    size_t dapsize; /* |dapdata|; this is transient */
    void* dap; /* pointer into rawdata where dap data starts */ 
    size_t dmrsize; /* |dmrdata| */
    char* dmr;/* pointer into rawdata where dmr starts */ 
    char* errdata; /* null || error chunk (null terminated) */
    int hostlittleendian; /* 1 if the host is little endian */
    int remotelittleendian; /* 1 if the packet says data is little endian */
    int nochecksum; /* 1 if the packet says no checksums are included */
} NCD4serial;

/* This will be passed out of the parse */
struct NCD4meta {
    NCD4INFO* controller;
    int ncid; /* root ncid of the substrate netcdf-4 file; copy of NCD4parse argument*/
    NCD4node* root;
    NClist* allnodes; /*list<NCD4node>*/
    struct Error { /* Content of any error response */
	char* parseerror;
	int   httpcode;
	char* message;
	char* context;
	char* otherinfo;
    } error;
    int debuglevel;
    NCD4serial serial;
    NCD4CSUM checksummode;
    int checksumming; /* 1=>compute local checksum */
    int swap; /* 1 => swap data */
#if 0
    /* To avoid lost memory, we keep lists of malloc'd chunks that might need to
       be reclaimed. */
    NClist* blobs;  /* remember blobs that need to be reclaimed if there was an error;
                           as a rule, these are the blobs constituting data being
                           returned to the client */
#endif
    /* Define some "global" (to a DMR) data */
    NClist* groupbyid; /* NClist<NCD4node*> indexed by groupid >> 16; this is global */
    NCD4node* _bytestring; /* If needed */
};

typedef struct NCD4parser {
    char* input;
    int debuglevel;
    NCD4meta* metadata;
    /* Capture useful subsets of dataset->allnodes */
    NClist* types; /*list<NCD4node>*/
    NClist* dims; /*list<NCD4node>*/
    NClist* vars; /*list<NCD4node>*/
    NClist* groups; /*list<NCD4node>*/
    /* Convenience for short cut fqn detection */
    NClist* atomictypes; /*list<NCD4node>*/
    NCD4node* dapopaque; /* Single non-fixed-size opaque type */
} NCD4parser;

/**************************************************/

typedef struct NCD4triple {
        char* host; /* includes port if specified */
        char* key;
        char* value;
} NCD4triple;


/**************************************************/

/* Collect global state info in one place */
struct NCD4globalstate {
    struct {
        int proto_file;
        int proto_https;
    } curl;
    char* tempdir; /* track a usable temp dir */
    char* home; /* track $HOME for use in creating $HOME/.oc dir */
    struct {
	int ignore; /* if 1, then do not use any rc file */
	int loaded;
        NClist* rc; /*NClist<NCD4triple>; the rc file triple store fields*/
        char* rcfile; /* specified rcfile; overrides anything else */
    } rc;
};

/* Curl info */
struct NCD4curl {
    CURL* curl; /* curl handle*/
    NCbytes* packet; 
    struct errdata {/* Hold info for an error return from server */
	char* code;
	char* message;
	long  httpcode;
	char  errorbuf[CURL_ERROR_SIZE]; /* CURLOPT_ERRORBUFFER*/
    } errdata;
    struct curlflags {
        int proto_file; /* Is file: supported? */
        int proto_https; /* is https: supported? */
	int compress; /*CURLOPT_ENCODING*/
	int verbose; /*CURLOPT_ENCODING*/
	int timeout; /*CURLOPT_TIMEOUT*/
	int maxredirs; /*CURLOPT_MAXREDIRS*/
	char* useragent; /*CURLOPT_USERAGENT*/
	/* track which of these are created by oc */
#define COOKIECREATED 1
#define NETRCCREATED 2
	int createdflags;
	char* cookiejar; /*CURLOPT_COOKIEJAR,CURLOPT_COOKIEFILE*/
	char* netrc; /*CURLOPT_NETRC,CURLOPT_NETRC_FILE*/
    } curlflags;
    struct ssl {
	int   verifypeer; /* CURLOPT_SSL_VERIFYPEER;
                             do not do this when cert might be self-signed
                             or temporarily incorrect */
	int   verifyhost; /* CURLOPT_SSL_VERIFYHOST; for client-side verification */
        char* certificate; /*CURLOPT_SSLCERT*/
	char* key; /*CURLOPT_SSLKEY*/
	char* keypasswd; /*CURLOPT_SSLKEYPASSWD*/
        char* cainfo; /* CURLOPT_CAINFO; certificate authority */
	char* capath;  /*CURLOPT_CAPATH*/
    } ssl;
    struct proxy {
	char *host; /*CURLOPT_PROXY*/
	int port; /*CURLOPT_PROXYPORT*/
	char* userpwd; /*CURLOPT_PROXYUSERPWD*/
    } proxy;
    struct credentials {
	char *userpwd; /*CURLOPT_USERPWD*/
    } creds;
};

/**************************************************/
/* Define a structure holding common info */

struct NCD4INFO {
    NC*   controller; /* Parent instance of NCD4INFO */
    int debug;
    char* rawurltext; /* as given to ncd4_open */
    char* urltext;    /* as modified by ncd4_open */
    NCURI* uri;      /* parse of rawuritext */
    NCD4curl* curl;
    int inmemory; /* store fetched data in memory? */
    struct {
	char*   memory;   /* allocated memory if ONDISK is not set */
        char*   ondiskfilename; /* If ONDISK is set */
        FILE*   ondiskfile;     /* ditto */
        d4size_t   datasize; /* size on disk or in memory */
        long dmrlastmodified;
        long daplastmodified;
    } data;
    struct {
	char* filename; /* of the substrate file */
        int nc4id; /* substrate nc4 file ncid used to hold metadata; not same as external id  */
	NCD4meta* metadata;
    } substrate;
    struct {
        NCCONTROLS  flags;
        NCCONTROLS  debugflags;
	NCD4translation translation;
	char substratename[NC_MAX_NAME];
    } controls;
};

#endif /*D4TYPES_H*/