#include <assert.h>
#include <inttypes.h>    // for PRIu64
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sm_internal.h"

static bool
is_prime_or_9_or_15( uint32_t x )
{
    if( x == 9 || x == 15 ) return true;
    for( uint32_t y = 2; y * y <= x; y++ )
    {
        if( x % y == 0 ) return false;
    }
    return true;
}

static uint32_t
next_prime_or_9_or_15( uint32_t x )
{
    while( 1 )
    {
        x++;
        if( is_prime_or_9_or_15( x ) ) return x;
    }
}

static uint32_t
next_prime_or_9_or_15_or_power_of_two( uint32_t x )
{
    while( 1 )
    {
        x++;
        if( is_prime_or_9_or_15( x ) ) return x;
        if( is_power_of_two( x ) ) return x;
    }
}

static uint32_t
ceil_log_2( uint64_t d )
{
    uint32_t result = is_power_of_two( d ) ? 0 : 1;
    while( d > 1 )
    {
        result++;
        d = d >> 1;
    }
    return result;
}

static uint32_t
calculate_shift_magic( uint64_t d )
{
    if( d > chunksize ) { return 1; }
    else if( is_power_of_two( d ) ) { return ceil_log_2( d ); }
    else { return 32 + ceil_log_2( d ); }
}

static uint64_t
calculate_multiply_magic( uint64_t d )
{
    if( d > chunksize ) { return 1; }
    else if( is_power_of_two( d ) ) { return 1; }
    else { return ( d - 1 + ( 1ull << calculate_shift_magic( d ) ) ) / d; }
}

static uint64_t
gcd( uint64_t a, uint64_t b )
{
    if( a == b ) return a;
    if( a < b ) return gcd( a, b - a );
    return gcd( b, a - b );
}

static uint64_t
lcm( uint64_t a, uint64_t b )
{
    uint64_t g = gcd( a, b );
    return ( a / g ) * b;
}

static uint64_t
calculate_foliosize( uint64_t objsize )
{
    if( objsize > chunksize ) return objsize;
    if( is_power_of_two( objsize ) )
    {
        if( objsize < pagesize )
            return pagesize;
        else
            return objsize;
    }
    if( objsize > 16 * 1024 ) return objsize;
    if( objsize > 256 ) { return ( objsize / cacheline_size ) * pagesize; }
    if( objsize > pagesize ) return objsize;
    return lcm( objsize, pagesize );
}

static void
print_number( FILE* f, uint64_t n, int len )
{
    fprintf( f, "%*" PRIu64, len, n );
    /*if( n <= 1024 || ( n % 1024 ) )
        fprintf( f, "%*" PRIu64, len, n );
    else if( n < 1024 * 1024 || ( n % ( 1024 * 1024 ) ) )
        fprintf( f, "%*" PRIu64 "*Ki", max( 1, len - 3 ), n / 1024 );
    else if( is_power_of_two( n ) )
        fprintf( f, "1" PRIu64 " << %*d", max( 1, len - 5 ), lg_of_power_of_two( n ) );
    else
        fprintf( f, "%*" PRIu64 "*Me", max( 1, len - 3 ), n / ( 1024 * 1024 ) );*/
}

enum bin_category
{
    BIN_SMALL,
    BIN_LARGE,
    BIN_HUGE
};

static uint32_t
calculate_overhead_pages_per_chunk( enum bin_category bc, uint64_t foliosize )
{
    switch( bc )
    {
        case BIN_HUGE: return 0;
        case BIN_LARGE: return 1;
        case BIN_SMALL: return ceil32( (uint32_t) ( sizeof( per_folio ) * ( chunksize / foliosize ) ), pagesize );
        default: abort();
    }
}

typedef struct static_bin_t
{
    uint64_t object_size;
    uint64_t foliosize;
    /* 
     * a folio is like a pagesize: we try to find the folio with the fewest free slots on it, 
     * when allocating storage. 
     */
    uint32_t objects_per_folio;
    uint64_t object_division_multiply_magic;
    uint64_t folio_division_multiply_magic;
    uint32_t object_division_shift_magic;
    uint32_t folio_division_shift_magic;
    uint32_t overhead_pages_per_chunk;
    uint32_t folios_per_chunk;
} static_bin_t;

void
static_bin_init( static_bin_t* inst, enum bin_category bc, uint64_t object_size )
{
    inst->object_size                    = object_size;
    inst->foliosize                      = calculate_foliosize( object_size );
    inst->objects_per_folio              = inst->foliosize / object_size;
    inst->object_division_multiply_magic = calculate_multiply_magic( object_size );
    inst->folio_division_multiply_magic  = calculate_multiply_magic( inst->foliosize );
    inst->object_division_shift_magic    = calculate_shift_magic( object_size );
    inst->folio_division_shift_magic     = calculate_shift_magic( inst->foliosize );
    inst->overhead_pages_per_chunk       = calculate_overhead_pages_per_chunk( bc, inst->foliosize );
    inst->folios_per_chunk =
        object_size < chunksize ? ( chunksize - inst->overhead_pages_per_chunk * pagesize ) / inst->foliosize : 1;

    assert( inst->objects_per_folio <= max_objects_per_folio );
}

void
static_bin_print( static_bin_t* inst, FILE* f, uint32_t bin )
{
    fprintf( f, "  { " );
    print_number( f, inst->object_size, 7 );
    fprintf( f, ", " );
    assert( inst->foliosize % 4096 == 0 );
    print_number( f, inst->foliosize, 10 );
    fprintf( f,
             ",              %4u,              %3u,                       %2u,                  %2u,          %2u,    "
             "%10" PRIu64 PRIu64 ",   %10" PRIu64 PRIu64 " },  // %3u",
             inst->objects_per_folio, inst->folios_per_chunk, inst->overhead_pages_per_chunk, inst->object_division_shift_magic,
             inst->folio_division_shift_magic, inst->object_division_multiply_magic, inst->folio_division_multiply_magic, bin );
}

int
main( int argc, const char* argv[] )
{
    /* objsizes generated_constants.cc > generated_constants.hxx */

    static_bin_t  b;
    static_bin_t* static_bins;
    const size_t  static_bin_cnt = 47;

    assert( argc == 2 );
    FILE* cf = fopen( argv[1], "w" );
    assert( cf );
    const char* name = "GENERATED_CONSTANTS_HXX_";
    fprintf( cf, "#include \"generated_constants.hxx\"\n" );
    printf( "#ifndef %s\n", name );
    printf( "#define %s\n", name );
    printf( "// Do not edit this file.  This file is automatically generated\n" );
    printf( "//  from %s\n\n", __FILE__ );
    fprintf( cf, "#include \"sm_internal.h\"\n" );
    printf( "#include \"sm_internal.h\"\n" );
    printf( "#include \"sm_config.h\"\n" );
    printf( "#include <stdlib.h>\n" );
    printf( "#include <stdint.h>\n" );
    printf( "#include <sys/types.h>\n" );
    printf( "// For chunks containing small objects, we reserve the first\n" );
    printf( "// several pages for bitmaps and linked lists.\n" );
    printf( "// There's a per_page struct containing 2 pointers and a\n" );
    printf( "// bitmap, and there are 512 of those.\n" );
    printf( "// As a result, there is no overhead in each page, but there is\n" );
    printf( "// overhead per chunk, which affects the large object sizes.\n\n" );
    printf( "// We obtain hugepages from the operating system via mmap(2).\n" );
    printf( "// By `hugepage', I mean only mmapped pages.\n" );
    printf( "// By `page', I mean only a page inside a hugepage.\n" );
    printf( "// Each hugepage has a bin number.\n" );

    printf( "// We use a static array to keep track of the bin of each a hugepage.\n" );
    printf( "//  Bins [0..first_huge_bin_number) give the size of an object.\n" );
    printf( "//  Larger bin numbers B indicate the object size, coded as\n" );
    printf( "//     malloc_usable_size(object) = page_size*(bin_of(object)-first_huge_bin_number;\n" );

    static_bins = (static_bin_t*) malloc( static_bin_cnt * sizeof( static_bin_t ) );
    assert( static_bins );

    const char* const struct_definition =
        "typedef struct static_bin_s { uint64_t object_size, folio_size; objects_per_folio_t objects_per_folio; "
        "folios_per_chunk_t "
        "folios_per_chunk;  uint8_t overhead_pages_per_chunk, object_division_shift_magic, folio_division_shift_magic; uint64_t "
        "object_division_multiply_magic, folio_division_multiply_magic;} static_bin_s";
    printf( "%s;\nextern static_bin_s static_bin_info[];\n", struct_definition );
    fprintf( cf, "SM_ALIGNED( 64 ) static_bin_s static_bin_info[] = { \n" );
    fprintf( cf, "// The first class of small objects try to get a maximum of 25%% internal fragmentation by having sizes of the "
                 "form c<<k where c is 4, 5, 6 or 7.\n" );
    fprintf( cf, "// We stop at when we have 4 cachelines, so that the ones that happen to be multiples of cache lines are "
                 "either a power of two or odd.\n" );
    const char* const header_line =
        "//{ objsize, folio_size, objects_per_folio, folios_per_chunk, overhead_pages_per_chunk, magic: object_shift, "
        "folio_shift, object_multiply, folio_multiply},  // fragmentation(overhead bins net)\n";
    fprintf( cf, "%s", header_line );

    int bin = 0;

    uint64_t prev_non_aligned_size = 8;    // a size that could be returned by a non-aligned malloc
    for( uint64_t k = 8; 1; k *= 2 )
    {
        for( uint64_t c = 4; c <= 7; c++ )
        {
            uint64_t objsize = ( c * k ) / 4;
            if( objsize > 4 * cacheline_size ) goto done_small;
            static_bin_init( &b, BIN_SMALL, objsize );
            double wasted = ( chunksize - b.folios_per_chunk * b.objects_per_folio * objsize ) / (double) chunksize;
            static_bin_print( &b, cf, bin );
            fprintf( cf, "    %3.1f%%", wasted * 100.0 );
            if( objsize == 8 || ( is_power_of_two( objsize ) && objsize > cacheline_size ) )
            {
                fprintf( cf, "       %4.1f%%\n", wasted * 100.0 );
            }
            else
            {
                double bin_wastage    = ( objsize - prev_non_aligned_size - 1 ) / (double) objsize;
                prev_non_aligned_size = objsize;
                fprintf( cf, " %4.1f%% %4.1f%%\n", bin_wastage * 100.0, ( ( 1 + bin_wastage ) * ( 1 + wasted ) - 1 ) * 100.0 );
            }

            assert( bin < static_bin_cnt );
            memcpy( &static_bins[bin++], &b, sizeof( static_bin_t ) );
        }
    }
done_small:

    fprintf( cf, "// Class 2 small objects are prime multiples of a cache line.\n" );
    fprintf( cf, "// The folio size is such that the number of 4K pages equals the\n" );
    fprintf( cf, "// number of cache lines in the object.  Namely, the folio size is 64 times\n" );
    fprintf( cf, "// the object size.  The small_chunk_header fits into 8 pages.\n" );

    fprintf( cf, "%s", header_line );

    uint32_t prev_objsize_in_cachelines = 4;
    size_t   largest_small              = 0;
    for( uint32_t objsize_in_cachelines = 5; objsize_in_cachelines < 64 * 4;
         objsize_in_cachelines          = next_prime_or_9_or_15_or_power_of_two( objsize_in_cachelines ) )
    {
        /*
         * Must trade off two kinds of internal fragmentation and external
         * fragmentation.
         * 
         * The external fragmentation is caused by having
         * too many bins, and I don't know how to quantify that except
         * that it seems reasonable to restrict us to having a O(log
         * chunksize) bins.
         * 
         * The internal fragmentation comes from two sources: (a) the
         * waste at the end of a chunk when the pages don't fit (which we
         * could mitigate by using many 2MB blocks together for a single
         * chunk, but it could grow out of hand), and (b) the fact that we
         * must round up to the next chunk when allocating memory.
         */
        if( is_power_of_two( objsize_in_cachelines )
            || prev_objsize_in_cachelines < 0.7 * next_prime_or_9_or_15( objsize_in_cachelines )    // not next power of two
        )
        {
            uint32_t objsize = objsize_in_cachelines * cacheline_size;
            static_bin_init( &b, BIN_SMALL, objsize );
            largest_small = objsize;
            // TODO: Don't like this magic number ('8 * pagesize' really?)
            uint32_t folios_per_chunk       = ( chunksize - 8 * pagesize ) / b.foliosize;
            uint32_t minimum_used_per_folio = ( prev_objsize_in_cachelines * cacheline_size + 1 ) * b.objects_per_folio;
            double   fragmentation          = ( folios_per_chunk * minimum_used_per_folio ) / (double) chunksize;
            static_bin_print( &b, cf, bin );
            fprintf( cf, "     %0.3f (%2d cache lines, %d folios/chunk, at least %d bytes used/folio)\n", fragmentation,
                     objsize_in_cachelines, folios_per_chunk, minimum_used_per_folio );
            assert( bin < static_bin_cnt );
            memcpy( &static_bins[bin++], &b, sizeof( static_bin_t ) );

            if( !is_power_of_two( objsize_in_cachelines ) ) { prev_objsize_in_cachelines = objsize_in_cachelines; }
        }
    }

    const uint64_t offset_of_first_object_in_large_chunk = pagesize;

    fprintf( cf, "// large objects (page allocated):\n" );
    fprintf( cf, "// So that we can return an accurate malloc_usable_size(), we maintain (in the first page of each largepage "
                 "chunk) information about each object (large_object_list_cell)\n" );
    fprintf( cf, "// For unallocated objects we maintain a next pointer to the next large_object_list_cell for an free object "
                 "of the same size.\n" );
    fprintf( cf, "// For allocated objects, we maintain the footprint.\n" );
    fprintf( cf, "// This extra information always fits within one page.\n" );
    uint32_t largest_waste_at_end = log_chunksize - 4;
    fprintf( cf,
             "// This introduces fragmentation.  This fragmentation doesn't matter much since it will be purged. For sizes up "
             "to 1<<%d we waste the last potential object.\n",
             largest_waste_at_end );
    fprintf( cf,
             "// for the larger stuff, we reduce the size of the object slightly which introduces some other fragmentation\n" );
    int first_large_bin = bin;
    for( uint64_t log_allocsize = 14; log_allocsize < log_chunksize; log_allocsize++ )
    {
        uint64_t    objsize = 1ull << log_allocsize;
        const char* comment = "";
        if( log_allocsize > largest_waste_at_end )
        {
            objsize -= pagesize;
            comment = " (reserve a page for the list of sizes)";
        }
        static_bin_init( &b, BIN_LARGE, objsize );
        static_bin_print( &b, cf, bin );
        fprintf( cf, " %s\n", comment );
        assert( bin < static_bin_cnt );
        memcpy( &static_bins[bin++], &b, sizeof( static_bin_t ) );

        assert( b.objects_per_folio * b.folios_per_chunk * sizeof( large_object_list_cell )
                <= offset_of_first_object_in_large_chunk );
    }
    binnumber_t first_huge_bin = bin;
    fprintf( cf, "// huge objects (chunk allocated) start  at this size. %d\n", chunksize );
    for( uint64_t siz = chunksize; siz < ( 1ull << 48 ); siz *= 2 )
    {
        static_bin_init( &b, BIN_HUGE, siz );
        static_bin_print( &b, cf, bin++ );
        fprintf( cf, "\n" );
    }
    fprintf( cf, "\n};\n" );

    printf( "enum { bin_number_limit = %u,\n", bin );
    printf( "    largest_small         = %" PRIu64 ",\n", largest_small );
    const size_t largest_large = ( 1 << ( log_chunksize - 1 ) ) - pagesize;
    printf( "    offset_of_first_object_in_large_chunk = %" PRIu64 ",\n", offset_of_first_object_in_large_chunk );
    printf( "    largest_large         = %" PRIu64 ",\n", largest_large );
    printf( "    first_large_bin_number = %d,\n", first_large_bin );
    printf( "    first_huge_bin_number   = %u };\n", first_huge_bin );

    printf( "#define REPEAT_FOR_SMALL_BINS(x) " );
    for( int b = 0; b < first_large_bin; b++ )
    {
        if( b > 0 ) printf( "," );
        printf( "x" );
    }

    printf( "\n" );

    printf( "struct per_folio; // Forward decl needed here.\n" );
    printf( "typedef struct dynamic_small_bin_info {\n" );
    printf( "  union {\n" );
    printf( "    struct {\n" );
    /*
     * Need two extra list for each folio.  One for the empty folios which have not been uncommitted 
     * (e.g., with madvise(MADV_DONTNEED)), and one for the empty folios that have been uncomitted.
     */
    int n_extra_lists_per_folio = 2;
    {
        int count = 0;
        for( int b = 0; b < first_large_bin; b++ )
        {
            printf( "      struct per_folio *b%d[%d];\n", b, static_bins[b].objects_per_folio + n_extra_lists_per_folio );
            assert( b < static_bin_cnt );
            count += static_bins[b].objects_per_folio + n_extra_lists_per_folio;
        }
        printf( "    };\n" );
        printf( "    struct per_folio *b[%d];\n", count );
    }
    printf( "  };\n" );
    printf( "} dynamic_small_bin_info;\n" );
    printf( "// dynamic_small_bin_offset is declared const even though one implementation looks in an array. The array is a "
            "const\n" );
    printf( "static inline uint32_t dynamic_small_bin_offset(binnumber_t bin) {\n" );
    printf( "    const static int offs[]={" );
    {
        int count = 0;
        for( int b = 0; b < first_large_bin; b++ )
        {
            if( b > 0 ) printf( ", " );
            printf( "%d", count );
            assert( b < static_bin_cnt );
            count += static_bins[b].objects_per_folio + n_extra_lists_per_folio;
        }
    }
    printf( "};\n" );
    printf( "    return offs[bin];\n" );
    printf( "}\n" );

    printf( "static inline binnumber_t size_2_bin(size_t size) {\n" );
    printf( "  if (size <= 8) return 0;\n" );
    printf( "  if (size <= 320) {\n" );
    printf( "    // bit hacking to calculate the bin number for the first group of small bins.\n" );
    printf( "    int nzeros = SM_BUILTIN_CLZ64(size);\n" );
    printf( "    size_t roundup = size + (1ull<<(61-nzeros))-1;\n" );
    printf( "    int nzeros2 = SM_BUILTIN_CLZ64(roundup);\n" );
    printf( "    return 4*(60-nzeros2)+ ((roundup>>(61-nzeros2))&3);\n" );
    printf( "  }\n" );
    for( binnumber_t bin_num = 0; bin_num < first_huge_bin; bin_num++ )
    {
        assert( bin_num < static_bin_cnt );
        if( static_bins[bin_num].object_size <= 320 ) printf( "  //" );
        printf( "  if (size <= %" PRIu64 ") return %d;\n", static_bins[bin_num].object_size, bin_num );
    }
    printf( "  if (size <= %d) return %d; // Special case to handle the values between the largest_large and chunksize/2\n",
            chunksize, first_huge_bin );
    printf( "  return %u + lg_of_power_of_two(hyperceil(size)) - log_chunksize;\n", first_huge_bin );
    printf( "}\n" );

    printf( "static inline size_t bin_2_size(binnumber_t bin) {\n" );
    printf( "  SM_ASSERT(bin < bin_number_limit);\n" );
    printf( "  return static_bin_info[bin].object_size;\n" );
    printf( "}\n\n" );

    printf( "static inline uint32_t divide_offset_by_objsize(uint32_t offset, binnumber_t bin) {\n" );
    printf( "  return (offset * static_bin_info[bin].object_division_multiply_magic) >> "
            "static_bin_info[bin].object_division_shift_magic;\n" );
    printf( "}\n\n" );
    printf( "static inline uint32_t divide_offset_by_foliosize(uint32_t offset, binnumber_t bin) {\n" );
    printf( "  return (offset * static_bin_info[bin].folio_division_multiply_magic) >> "
            "static_bin_info[bin].folio_division_shift_magic;\n" );
    printf( "}\n\n" );

    printf( "#endif\n" );

    free( static_bins );

    fclose( cf );

    return 0;
}
