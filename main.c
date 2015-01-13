// Linux: compile with gcc: gcc main.c -o das_editor -Wall
// Windows x86 built with mingw-w64.

// Standard C libs
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

// POSIX
#include <sys/stat.h>

// Platform specific librarys
#ifdef _WIN32
#	include <windows.h>
#endif

struct handle {
	int   hash;
	int   offset;
	float fp_val;
	float min;
	float max;
	char  name[32];
};

struct header {
	int       size;
	int       data_size;
	int       checksum;
	int       item_count;
	char      player_name[24];
	char      player_race[8];
	char      player_gender[8];
	float     total_play_time;
	char      player_id[64];
	char      player_class[16];
	int       player_level;
	char      player_pos[32];
	char      area_id[16];
	char      player_location[64];
	char      thumbnail[64];
	char      assets[128];
	long long timestamp;
	char      build_number[16];
	int       patch_number;
	int       addon_count;
	char      addons[32][128];
	int       data_checksum;
};

struct xml_file {
	int*			size;
	int*			offset;
	int*			shift_amount;
	unsigned char**	data;
};

int is_little_endian( void );
int char_to_file( const char*, unsigned char*, size_t );
int das_rw_values( unsigned char*, int, int );
int das_manual_write( struct handle, unsigned char* );
int das_file_export( const char*, struct handle*, int );
int das_import_file_write( const char*, struct handle*, int, unsigned char* );
const char* das_str_lookup( int );
unsigned char* file_to_char( const char*, size_t* );
void enter_to_continue( void );
struct handle das_set_struct( unsigned char*, const char[32], int, float, float );
struct header das_read_header( unsigned char* );
char* das_ts_to_str( long long );
char* das_pt_to_str( float );
int xml_mem_to_file( unsigned char*, int, const char* );
int das_dump_xmls( unsigned char*, int );

int main( int argc, char* argv[] ) {

	// Input string
	char in_str[10] = "";

	// Quick endian check
	if ( !is_little_endian() ) {
		fprintf( stderr, ":: ERROR: This program is not compatible with big endian machines.\n" );
		enter_to_continue();
		return( EXIT_FAILURE );
	}

	// Check input arguments
	if ( argc < 2 ) {
		fprintf( stderr, ":: ERROR: No file specified.\n" );
		enter_to_continue();
		return( EXIT_FAILURE );
	} else if ( argc > 3 ) {
		fprintf( stderr, ":: ERROR: Too many arguments.\n" );
		fprintf( stderr, "::  USAGE: ./dai_save_file <filename>.DAS\n" );
		enter_to_continue();
		return( EXIT_FAILURE );
	}

	// Is input actually a file?
	printf( ":: Opening file...\n" );
	struct stat sb;
	if ( stat( argv[1], &sb ) == -1 ) {
		fprintf( stderr, ":: ERROR: Cannot stat file.\n" );
		enter_to_continue();
		return( EXIT_FAILURE );
	}
	switch ( sb.st_mode & S_IFMT ) {
		case S_IFREG:
			break;
		default:
			fprintf( stderr, ":: ERROR: \"%s\" is not a file.\n", argv[1] );
			enter_to_continue();
			return( EXIT_FAILURE );
	}

	// Don't proceed if filesize is > 2mb or < 100kb
	if ( sb.st_size > 2000000 || sb.st_size < 100000 ) {
		fprintf( stderr, ":: ERROR: File size is off. Not a save file.\n" );
		enter_to_continue();
		return( EXIT_FAILURE );
	}

	// Read file into a memory buffer
	unsigned char* data = file_to_char( argv[1], NULL );
	if ( data == NULL ) {
		enter_to_continue();
		return( EXIT_FAILURE );
	}
	printf( "::  File loaded successfully.\n" );

	// Read header block
	struct header header = das_read_header( data );
	if ( header.size == 0 ) {
		fprintf( stderr, ":: ERROR: Cannot read file header.\n" );
		free( data );
		enter_to_continue();
		return( EXIT_FAILURE );
	}
	printf( "::\n:: %s (%s)\n",
		header.player_name,
		header.player_id );
	printf( ":: Level %d %s %s %s\n",
		header.player_level,
		header.player_race,
		header.player_gender,
		header.player_class );
	printf( ":: %s, %s\n",
		header.player_location,
		header.player_pos );
	printf( ":: Build %s, Patch %d\n",
		header.build_number,
		header.patch_number );
	printf( ":: Play Time: %s, Save Date: %s",
		das_pt_to_str( header.total_play_time ),
		das_ts_to_str( header.timestamp ) );

	// This will be the name of the output file.
	char new_filename[strlen(argv[1])+5];
	snprintf( new_filename, strlen(argv[1])+5, "%s.NEW", argv[1] );

	// Main menu
	printf( "::\n" );
	printf( "::  ------------ MENU ------------ \n" );
	printf( ":: | 0 : QUIT                     |\n" );
	printf( ":: | 1 : EXPORT VALUES TO FILE    |\n" );
	printf( ":: | 2 : IMPORT VALUES FROM FILE  |\n" );
	printf( ":: | 3 : MANUAL EDIT VALUES       |\n" );
	printf( ":: | 4 : EXPORT XML FILES         |\n" );
	printf( "::  ------------------------------ \n" );
	printf( ":: Enter an option [0/1/2/3/4] " );
	fgets( in_str, 10, stdin );
	switch ( in_str[0] ) {
		case '0':
			printf( ":: Quitting...\n" );
			free( data );
			enter_to_continue();
			return( EXIT_SUCCESS );
			break;
		case '1': // Export
			printf( ":: Exporting values to file...\n" );
			if ( das_rw_values( data, sb.st_size, 1 ) == -1 ) {
				free( data );
				enter_to_continue();
				return( EXIT_FAILURE );
			}
			break;
		case '2': // Import
			printf( ":: Importing values from file...\n" );
			if ( das_rw_values( data, sb.st_size, 2 ) == -1 ) {
				free( data );
				enter_to_continue();
				return( EXIT_FAILURE );
			}
			// Data has been edited. Write to file.
			if ( char_to_file( new_filename, data, sb.st_size ) == -1 ) {
				free( data );
				enter_to_continue();
				return( EXIT_FAILURE );
			}
			printf( ":: New save file written to %s\n", new_filename );
			break;
		case '3': // Manual writing of values
			printf( ":: Manual editing all values...\n::\n" );
			if ( das_rw_values( data, sb.st_size, 3 ) == -1 ) {
				free( data );
				enter_to_continue();
				return( EXIT_FAILURE );
			}
			// Data has been edited. Write to file.
			if ( char_to_file( new_filename, data, sb.st_size ) == -1 ) {
				free( data );
				enter_to_continue();
				return( EXIT_FAILURE );
			}
			printf( ":: New save file written to %s\n", new_filename );
			break;
		case '4':
			printf( ":: Exporting XML files...\n" );
			if ( das_dump_xmls( data, sb.st_size ) == -1 ) {
				free( data );
				enter_to_continue();
				return( EXIT_FAILURE );
			}
			break;
		default:
			fprintf( stderr, ":: ERROR: Incorrect option selected.\n" );
			free( data );
			enter_to_continue();
			return( EXIT_SUCCESS );
			break;
	}

	// Let user read the console and hit enter before closing
	// Free memory and quit
	free( data );
	enter_to_continue();
	return( EXIT_SUCCESS );

}

void enter_to_continue( void ) {

	char in_str[4];
	printf( "::\n:: Press ENTER to continue..." );
	fgets( in_str, 4, stdin );

}

int is_little_endian( void ) {

	short int number = 0x1;
	unsigned char* num = (unsigned char*)&number;
	return( num[0] == 1 );

}

unsigned char* file_to_char( const char* filename, size_t* filesize ) {

	// Open file if it exists, else return NULL
	FILE * fp;
	if ( !( fp = fopen( filename, "rb" ) ) ) {
		fprintf( stderr, ":: ERROR: Cannot open file \"%s\"\n", filename );
		return( NULL );
	}

	// Get the files size
	fseek( fp, 0, SEEK_END );
	size_t fsize = ftell( fp );
	rewind( fp );

	// Allocate memory to a buffer, +1 for null terminate
	unsigned char* buffer = (unsigned char*)malloc( ( fsize ) * sizeof( unsigned char ) );
	if ( buffer == NULL ) {
		fprintf( stderr, ":: ERROR: Memory allocation error.\n" );
		fclose( fp );
		return( NULL );
	}

	// Copy file data into the buffer
	if ( fread( buffer, sizeof( unsigned char ), fsize, fp ) != fsize ) {
		fprintf( stderr, ":: ERROR: File read error.\n" );
		fclose( fp );
		free( buffer );
		return( NULL );
	}

	// Cleanup and return
	fclose( fp );
	if ( filesize != NULL )
		*filesize = fsize;
	return( buffer );

}

int char_to_file( const char* filename, unsigned char* data, size_t filesize ) {

	// Were we passed something valid?
	if ( data == NULL )
		return( -1 );

	// Open file for write
	FILE * fp;
	if ( !( fp = fopen( filename, "wb" ) ) ) {
		fprintf( stderr, ":: ERROR: Cannot create file \"%s\"\n", filename );
		return( -1 );
	}

	// Write data to file
	if ( fwrite( data, sizeof( unsigned char ), filesize, fp ) != filesize ) {
		fprintf( stderr, ":: ERROR: writing %s, file may be corrupted.\n", filename );
		fclose( fp );
		return( -1 );
	}

	// Cleanup and return
	fclose( fp );
	return( 0 );

}

char* das_ts_to_str( long long ts ) {

	time_t rawtime = ts - ( 2440588LL * 24 * 60 * 60 );
	struct tm* timeinfo = gmtime( &rawtime );
	return( asctime( timeinfo ) );

}

char* das_pt_to_str( float play_time ) {

	static char buf[64];
	int seconds = (int)play_time % 60;
	int minutes = ( (int)play_time / 60 ) % 60;
	int hours = (int)play_time / 3600;
	snprintf( buf, 64, "%d:%.2d:%.2d", hours, minutes, seconds );
	return( buf );

}

struct header das_read_header( unsigned char* data ) {

	// Initialize struct
	struct header header = {
		.size            = 0,
		.data_size       = 0,
		.checksum        = 0,
		.item_count      = 0,
		.player_name     = "",
		.player_race     = "",
		.player_gender   = "",
		.total_play_time = 0,
		.player_id       = "",
		.player_class    = "",
		.player_level    = 0,
		.player_pos      = "",
		.area_id         = "",
		.player_location = "",
		.thumbnail       = "",
		.assets          = "",
		.timestamp       = 0,
		.build_number    = "",
		.patch_number    = 0,
		.addon_count     = 0,
		.data_checksum   = 0
	};

	// Vars
	int i = 0, d = 0, ind = 0, offset = 0;

	// Start reading file and printing info.
	// Check first 10 bytes for FBCHUNKS file
	if (	data[0] != 0x46 ||  // F
			data[1] != 0x42 ||  // B
			data[2] != 0x43 ||  // C
			data[3] != 0x48 ||  // H
			data[4] != 0x55 ||  // U
			data[5] != 0x4E ||  // N
			data[6] != 0x4B ||  // K
			data[7] != 0x53 ||  // S
			data[8] != 0x01 ||  // <SOH>
			data[9] != 0x00 ) { // \0
		fprintf( stderr, "  ERROR: Not a valid save file.\n" );
		return( header );
	}
	offset += 10;

	// At offset 0xA, 4 bytes, size of header in bytes
	for ( i = 3; i >= 0; i-- )
		header.size = ( header.size << 8 ) + data[offset+i];
	offset += 4;

	// At offset 0xE, 4 bytes, size of data in bytes
	for ( i = 3; i >= 0; i-- )
		header.data_size = ( header.data_size << 8 ) + data[offset+i];
	offset += 4;

	// At offset 0x12, 4 bytes, header checksum
	for ( i = 3; i >= 0; i-- )
		header.checksum = ( header.checksum << 8 ) + data[offset+i];
	offset += 4;

	// We don't care about these 10 bytes, so move on
	// 46 42 48 45 41 44 45 52 00 01
	// F  B  H  E  A  D  E  R  \0 SOH
	offset += 10;

	// At offset 0x20, 4 bytes, number of items to loop (big endian)
	for ( i = 0; i < 4; i++ )
		header.item_count = ( header.item_count << 8 ) + data[offset+i];
	offset += 4;

	// Allocate total item data array
	unsigned char** item_data =
		(unsigned char**)malloc( header.item_count * sizeof( unsigned char* ) );

	// At offset 0x24, start loop item_count times
	// Each item starts with 4 bytes (unknown hash) 2 bytes length, item data
	for ( i = 0; i < header.item_count; i++ ) {
		int item_hash = 0;
		for ( d = 0; d < 4; d++ )
			item_hash = ( item_hash << 8 ) + data[offset+d];
		offset += 4;
		// 2 bytes, items (string) length
		int item_length = 0;
		for ( d = 0; d < 2; d++ )
			item_length = ( item_length << 8 ) + data[offset+d];
		offset += 2;
		item_data[i] = ( unsigned char* )malloc( ( item_length + 1 ) * sizeof( unsigned char ) );
		item_data[i] = ( unsigned char* )memcpy( item_data[i], data + offset, item_length );
		memset( item_data[i] + item_length, 0, 1 );
		switch ( item_hash ) {
			case 0x92796772: // Player Name
				snprintf( header.player_name, 24, "%s", item_data[i] );
				break;
			case 0x926E6FA0: // Race, 0 = human, 1 = elf, 2 = dwarf, 3 = qunari
				switch ( item_data[i][0] ) {
					case 0x30:
						snprintf( header.player_race, 8, "Human" );
						break;
					case 0x31:
						snprintf( header.player_race, 8, "Elf" );
						break;
					case 0x32:
						snprintf( header.player_race, 8, "Dwarf" );
						break;
					case 0x33:
						snprintf( header.player_race, 8, "Qunari" );
						break;
					default:
						snprintf( header.player_race, 8, "Unknown" );
						break;
				}
				break;
			case 0x06AE718A: // Gender, 0 = male, 1 = female
				switch ( item_data[i][0] ) {
					case 0x30:
						snprintf( header.player_gender, 8, "Male" );
						break;
					case 0x31:
						snprintf( header.player_gender, 8, "Female" );
						break;
					default:
						snprintf( header.player_gender, 8, "Unknown" );
						break;
				}
				break;
			case 0x979CAD3D: // Total play time in seconds (float)
				header.total_play_time = atof( (const char*)item_data[i] );
				break;
			case 0xB615BDD8: // 128 bit character id
				snprintf( header.player_id, 64, "%s", item_data[i] );
				break;
			case 0xE17097FB: // Class, 1 = warrior, 2 = rogue, 3 = mage
				switch ( item_data[i][0] ) {
					case 0x31:
						snprintf( header.player_class, 16, "Warrior" );
						break;
					case 0x32:
						snprintf( header.player_class, 16, "Rogue" );
						break;
					case 0x33:
						snprintf( header.player_class, 16, "Mage" );
						break;
					default:
						snprintf( header.player_class, 16, "Unknown" );
						break;
				}
				break;
			case 0xE1CA2F03: // Level
				header.player_level = atoi( (const char*)item_data[i] );
				break;
			case 0xA521BDF0: // Position in world
				snprintf( header.player_pos, 32, "%s", item_data[i] );
				break;
			case 0x5F500F34: //  This may be some location code (area id)
				snprintf( header.area_id, 16, "%s", item_data[i] );
				break;
			case 0x2F852FB8: // Location
				snprintf( header.player_location, 64, "%s", item_data[i] );
				break;
			case 0xB3991F9F: // Save thumbnail image
				snprintf( header.thumbnail, 64, "%s", item_data[i] );
				break;
			case 0x9A832D89: // Map assets
				snprintf( header.assets, 128, "%s", item_data[i] );
				break;
			case 0x39AA8AB0: // Time of save
				header.timestamp = atoll( (const char*)item_data[i] );
				break;
			case 0x8509F5B0: // Game exe build number (version.json)
				snprintf( header.build_number, 16, "%s", item_data[i] );
				break;
			case 0x0346EAF1: // Patch level
				header.patch_number = atoi( (const char*)item_data[i] );
				break;
			case 0x4D86FB47: // Loaded addons with ~ as seperator
				ind = 0;
				for ( d = 0; d < item_length - 1; d++ ) {
					if ( item_data[i][d] == 0x7E ) {
						header.addon_count++;
						header.addons[header.addon_count][ind] = '\0';
						ind = 0;
					} else {
						header.addons[header.addon_count][ind] = item_data[i][d];
						ind++;
					}
				}
				break;
			default: // Something I don't know
				break;
		}
		offset += item_length;
	}

	// At current offset, 4 bytes,
	for ( i = 3; i >= 0; i-- )
		header.data_checksum = ( header.data_checksum << 8 ) + data[offset+i];

	// Cleanup and return
	for ( i = 0; i < header.item_count; i++ )
		free( item_data[i] );
	free( item_data );
	return( header );

}

const char* das_str_lookup( int index ) {

	if ( index < 0 || index > 53 )
		return( NULL );

	static const char lookup[54][32] = {
		"EYELINER_INTENSITY",      // 00
		"EYELINER COLOR RED",      // 01
		"EYELINER COLOR GREEN",    // 02
		"EYELINER COLOR BLUE",     // 03
		"EYE SHADOW INTENSITY",    // 04
		"EYE SHADOW COLOR RED",    // 05
		"EYE SHADOW COLOR GREEN",  // 06
		"EYE SHADOW COLOR BLUE",   // 07
		"UNDER-EYE COLOR RED",     // 08
		"UNDER-EYE COLOR GREEN",   // 09
		"UNDER-EYE COLOR BLUE",    // 10
		"BLUSH INTENSITY",         // 11
		"BLUSH COLOR RED",         // 12
		"BLUSH COLOR GREEN",       // 13
		"BLUSH COLOR BLUE",        // 14
		"LIP SHINE",               // 15
		"LIP INTENSITY",           // 16
		"LIP COLOR RED",           // 17
		"LIP COLOR GREEN",         // 18
		"LIP COLOR BLUE",          // 19
		"LIP LINER COLOR RED",     // 20
		"LIP LINER COLOR GREEN",   // 21
		"LIP LINER COLOR BLUE",    // 22
		"UNDER-BROW INTENSITY",    // 23
		"UNDER-BROW COLOR RED",    // 24
		"UNDER-BROW COLOR GREEN",  // 25
		"UNDER-BROW COLOR BLUE",   // 26
		"EYEBROW COLOR RED",       // 27
		"EYEBROW COLOR GREEN",     // 28
		"EYEBROW COLOR BLUE",      // 29
		"EYELASH COLOR RED",       // 30
		"EYELASH COLOR GREEN",     // 31
		"EYELASH COLOR BLUE",      // 32
		"HAIR COLOR RED",          // 33
		"HAIR COLOR GREEN",        // 34
		"HAIR COLOR BLUE",         // 35
		"HAIR SPEC1 COLOR RED",    // 36
		"HAIR SPEC1 COLOR GREEN",  // 37
		"HAIR SPEC1 COLOR BLUE",   // 38
		"HAIR SPEC2 COLOR RED",    // 39
		"HAIR SPEC2 COLOR GREEN",  // 40
		"HAIR SPEC2 COLOR BLUE",   // 41
		"SCALP HAIR COLOR RED",    // 42
		"SCALP HAIR COLOR GREEN",  // 43
		"SCALP HAIR COLOR BLUE",   // 44
		"FACIAL HAIR COLOR RED",   // 45
		"FACIAL HAIR COLOR GREEN", // 46
		"FACIAL HAIR COLOR BLUE",  // 47
		"INNER IRIS COLOR RED",    // 48
		"INNER IRIS COLOR GREEN",  // 49
		"INNER IRIS COLOR BLUE",   // 50
		"OUTER IRIS COLOR RED",    // 51
		"OUTER IRIS COLOR GREEN",  // 52
		"OUTER IRIS COLOR BLUE"    // 53
	};

	return( lookup[index] );

}

int das_rw_values( unsigned char* data, int filesize, int setting ) {

	// Variables
	int offset = 0, hash = 0, shift_count = -1, d = 0, i = 0;
	unsigned char tbit = 0;

	// First, shift the data, or return if we can't find anything.
	// 5 byte str we are looking for
	unsigned char xmlstr[5] = { 0x3C, 0x3F, 0x78, 0x6D, 0x6C };

	// Bit shift the file until the find a readable XML file
	// Loop 7 times, 1 bit shift to the right at a time
	for ( d = 0; d < 8; d++ ) {
		// Scan 5 bytes at a time for "<?xml" until end of file
		for ( i = 0; i < filesize - 5; i++ )
			if ( memcmp( data + i, xmlstr, 5 ) == 0 )
				shift_count = d;
		if ( shift_count != -1 )
			break;
		// Shift file 1 bit to the right
		tbit = ( data[0] >> 1 ) | ( data[filesize-1] << 7 );
		for ( i = filesize - 1; i > 0; i-- )
			data[i] = ( data[i] >> 1 ) | ( data[i-1] << 7 );
		data[0] = tbit;
	}

	// Shift should be set at this point
	if ( shift_count == -1 ) {
		fprintf( stderr, ":: ERROR: Could not find face data in file.\n" );
		return( -1 );
	}

	// Initalize structs that hold the data
	int num_values = 0;
	for ( i = 0; das_str_lookup( i ) != NULL; i++ )
		num_values++;
	if ( num_values <= 0 ) { // this shouldnt be possible
		fprintf( stderr, ":: ERROR: No values in lookup table.\n" );
		return( -1 );
	}
	struct handle value[num_values];
	for ( i = 0; i < num_values; i++ ) {
		value[i].hash   = 0;
		value[i].offset = -1;
		value[i].fp_val = 0;
		value[i].min    = 0;
		value[i].max    = 0;
		snprintf( value[i].name, 32, "%s", das_str_lookup( i ) );
	}

	// Search for values and store location & value
	for ( offset = 4; offset < filesize - 4; offset++ ) {
		// Break the loop if the xml file starts
		// We need to do this because custom Hawkes
		if ( memcmp( data + offset, xmlstr, 5 ) == 0 )
			break;
		// Set 4 bytes as hash
		for ( i = 0; i < 4; i++ )
			hash = ( hash << 8 ) + data[offset+i];
		switch ( hash ) {
			case 0x4560EB1D:
				// Eyeliner Intensity
				value[0] = das_set_struct( data, value[0].name, offset+=4, 0.0f, 1.0f );
				// Eye Shadow Intensity
				value[4] = das_set_struct( data, value[4].name, offset+=4, 0.0f, 1.0f );
				// Blush Intensity
				value[11] = das_set_struct( data, value[11].name, offset+=4, 0.0f, 1.0f );
				// Lip Intensity
				value[16] = das_set_struct( data, value[16].name, offset+=4, 0.0f, 1.0f );
				break;
			case 0xB4E2B58B:
				// Lip Shine
				value[15] = das_set_struct( data, value[15].name, offset+=8, 0.0f, 1.0f );
				// Under-Brow Intensity
				value[23] = das_set_struct( data, value[23].name, offset+=8, 0.0f, 1.0f );
				break;
			case 0x982C9069:
				// Eyebrow Color RED
				value[27] = das_set_struct( data, value[27].name, offset+=4, 0.0f, 1.0f );
				// Eyebrow Color GREEN
				value[28] = das_set_struct( data, value[28].name, offset+=4, 0.0f, 1.0f );
				// Eyebrow Color BLUE
				value[29] = das_set_struct( data, value[29].name, offset+=4, 0.0f, 1.0f );
				break;
			case 0x5923F86C:
				// Blush Color RED
				value[12] = das_set_struct( data, value[12].name, offset+=4, 0.0f, 1.0f );
				// Blush Color GREEN
				value[13] = das_set_struct( data, value[13].name, offset+=4, 0.0f, 1.0f );
				// Blush Color BLUE
				value[14] = das_set_struct( data, value[14].name, offset+=4, 0.0f, 1.0f );
				break;
			case 0x27E256DA:
				// Eyeliner Color RED
				value[1] = das_set_struct( data, value[1].name, offset+=4, 0.0f, 1.0f );
				// Eyeliner Color GREEN
				value[2] = das_set_struct( data, value[2].name, offset+=4, 0.0f, 1.0f );
				// Eyeliner Color BLUE
				value[3] = das_set_struct( data, value[3].name, offset+=4, 0.0f, 1.0f );
				break;
			case 0xCBF6A094:
				// Under-Eye Color RED
				value[8] = das_set_struct( data, value[8].name, offset+=4, 0.0f, 1.0f );
				// Under-Eye Color GREEN
				value[9] = das_set_struct( data, value[9].name, offset+=4, 0.0f, 1.0f );
				// Under-Eye Color BLUE
				value[10] = das_set_struct( data, value[10].name, offset+=4, 0.0f, 1.0f );
				break;
			case 0xCBF6A097:
				// Eye Shadow Color RED
				value[5] = das_set_struct( data, value[5].name, offset+=4, 0.0f, 1.0f );
				// Eye Shadow Color GREEN
				value[6] = das_set_struct( data, value[6].name, offset+=4, 0.0f, 1.0f );
				// Eye Shadow Color BLUE
				value[7] = das_set_struct( data, value[7].name, offset+=4, 0.0f, 1.0f );
				break;
			case 0x181A7B16:
				// Lip Liner Color RED
				value[20] = das_set_struct( data, value[20].name, offset+=4, 0.0f, 1.0f );
				// Lip Liner Color GREEN
				value[21] = das_set_struct( data, value[21].name, offset+=4, 0.0f, 1.0f );
				// Lip Liner Color BLUE
				value[22] = das_set_struct( data, value[22].name, offset+=4, 0.0f, 1.0f );
				break;
			case 0x3C775B99:
				// Lip Color RED
				value[17] = das_set_struct( data, value[17].name, offset+=4, 0.0f, 1.0f );
				// Lip Color GREEN
				value[18] = das_set_struct( data, value[18].name, offset+=4, 0.0f, 1.0f );
				// Lip Color BLUE
				value[19] = das_set_struct( data, value[19].name, offset+=4, 0.0f, 1.0f );
				break;
			case 0x319BF572:
				// Scalp Color RED
				value[42] = das_set_struct( data, value[42].name, offset+=4, 0.0f, 1.0f );
				// Scalp Color GREEN
				value[43] = das_set_struct( data, value[43].name, offset+=4, 0.0f, 1.0f );
				// Scalp Color BLUE
				value[44] = das_set_struct( data, value[44].name, offset+=4, 0.0f, 1.0f );
				break;
			case 0x99631BDF:
				// Under-Brow Color RED
				value[24] = das_set_struct( data, value[24].name, offset+=4, 0.0f, 1.0f );
				// Under-Brow Color GREEN
				value[25] = das_set_struct( data, value[25].name, offset+=4, 0.0f, 1.0f );
				// Under-Brow Color BLUE
				value[26] = das_set_struct( data, value[26].name, offset+=4, 0.0f, 1.0f );
				break;
			case 0x860AD2A3:
				// Facial Hair Color RED
				value[45] = das_set_struct( data, value[45].name, offset+=4, 0.0f, 1.0f );
				// Facial Hair Color GREEN
				value[46] = das_set_struct( data, value[46].name, offset+=4, 0.0f, 1.0f );
				// Facial Hair Color BLUE
				value[47] = das_set_struct( data, value[47].name, offset+=4, 0.0f, 1.0f );
				break;
			case 0x3BD3FFDC:
				// Inner Iris Color RED
				value[48] = das_set_struct( data, value[48].name, offset+=4, 0.0f, 1.0f );
				// Inner Iris Color GREEN
				value[49] = das_set_struct( data, value[49].name, offset+=4, 0.0f, 1.0f );
				// Inner Iris Color BLUE
				value[50] = das_set_struct( data, value[50].name, offset+=4, 0.0f, 1.0f );
				break;
			case 0x75D44C82:
				// Outer Iris Color RED
				value[51] = das_set_struct( data, value[51].name, offset+=4, 0.0f, 1.0f );
				// Outer Iris Color GREEN
				value[52] = das_set_struct( data, value[52].name, offset+=4, 0.0f, 1.0f );
				// Outer Iris Color BLUE
				value[53] = das_set_struct( data, value[53].name, offset+=4, 0.0f, 1.0f );
				break;
			case 0x0C2A5CEE:
				// Eyelash Color RED
				value[30] = das_set_struct( data, value[30].name, offset+=4, 0.0f, 1.0f );
				// Eyelash Color GREEN
				value[31] = das_set_struct( data, value[31].name, offset+=4, 0.0f, 1.0f );
				// Eyelash Color BLUE
				value[32] = das_set_struct( data, value[32].name, offset+=4, 0.0f, 1.0f );
				break;
			case 0x6B2444AA:
				// Hair Color RED
				value[33] = das_set_struct( data, value[33].name, offset+=4, 0.0f, 1.0f );
				// Hair Color GREEN
				value[34] = das_set_struct( data, value[34].name, offset+=4, 0.0f, 1.0f );
				// Hair Color BLUE
				value[35] = das_set_struct( data, value[35].name, offset+=4, 0.0f, 1.0f );
				break;
			case 0x4BFD7239:
				// Hair Spec1 Color RED
				value[36] = das_set_struct( data, value[36].name, offset+=4, 0.0f, 1.0f );
				// Hair Spec1 Color GREEN
				value[37] = das_set_struct( data, value[37].name, offset+=4, 0.0f, 1.0f );
				// Hair Spec1 Color BLUE
				value[38] = das_set_struct( data, value[38].name, offset+=4, 0.0f, 1.0f );
				break;
			case 0xF63C0CBA:
				// Hair Spec2 Color RED
				value[39] = das_set_struct( data, value[39].name, offset+=4, 0.0f, 1.0f );
				// Hair Spec2 Color GREEN
				value[40] = das_set_struct( data, value[40].name, offset+=4, 0.0f, 1.0f );
				// Hair Spec2 Color BLUE
				value[41] = das_set_struct( data, value[41].name, offset+=4, 0.0f, 1.0f );
				break;
			default:
				break;
		}
	}

	// Do something with the data
	char out_fn[1024] = {0};
	switch ( setting ) {
		case 1: // Export values to a file
			printf( ":: Enter name of file to save [*.DASFACE]: " );
			fgets( out_fn, 1024, stdin );
			while ( out_fn[0] == '\n' || out_fn[0] == 0 || out_fn[0] == EOF ) {
				printf( ":: ERROR: Bad filename. Try again.\n");
				printf( ":: Enter name of file to save [*.DASFACE]: " );
				fgets( out_fn, 1024, stdin );
			}
			// Replace newline with file extension
			for( i = 0; i < 1014; i++ )
				if ( out_fn[i] == '\n' ) {
					out_fn[i] = '.';
					out_fn[i+1] = 'D';
					out_fn[i+2] = 'A';
					out_fn[i+3] = 'S';
					out_fn[i+4] = 'F';
					out_fn[i+5] = 'A';
					out_fn[i+6] = 'C';
					out_fn[i+7] = 'E';
					out_fn[i+8] = 0;
				}
			das_file_export( out_fn, value, num_values );
			break;
		case 2: // Import values from a file
			printf( ":: Enter path/name of file to open [*.DASFACE]: " );
			fgets( out_fn, 1024, stdin );
			while ( out_fn[0] == '\n' || out_fn[0] == 0 || out_fn[0] == EOF ) {
				printf( ":: ERROR: Bad filename. Try again.\n");
				printf( ":: Enter name of file to save [*.DASFACE]: " );
				fgets( out_fn, 1024, stdin );
			}
			// Replace newline with file extension
			for( i = 0; i < 1014; i++ )
				if ( out_fn[i] == '\n' ) {
					out_fn[i] = '.';
					out_fn[i+1] = 'D';
					out_fn[i+2] = 'A';
					out_fn[i+3] = 'S';
					out_fn[i+4] = 'F';
					out_fn[i+5] = 'A';
					out_fn[i+6] = 'C';
					out_fn[i+7] = 'E';
					out_fn[i+8] = 0;
				}
			das_import_file_write( out_fn, value, num_values, data );
			break;
		case 3: // Get user input and fill values
			for( i = 0; i < num_values; i++ )
				das_manual_write( value[i], data );
			break;
		default:
			break;
	}

	// Shift bits back to original and return
	tbit = ( data[filesize - 1] << shift_count ) | ( data[0] >> ( 8 - shift_count ) );
	for ( i = 0; i < filesize - 1; i++ )
		data[i] = ( data[i] << shift_count ) | ( data[i+1] >> ( 8 - shift_count ) );
	data[filesize - 1] = tbit;
	return( 0 );

}

int das_manual_write( struct handle hb, unsigned char* data ) {

	// If value wasn't found, offset should be -1
	if ( hb.offset == -1 )
		return( -1 );

	// Interactive value editing from stdin
	char in_str[10];
	printf( "::  %s\n::    Current=\"%f\" min=\"%f\" max=\"%f\": ", hb.name, hb.fp_val, hb.min, hb.max );
	fgets( in_str, 10, stdin );
	if ( in_str[0] == '\n' ) {
		printf( "::    Unchanged.\n" );
	} else {
		sscanf( in_str, "%f", &hb.fp_val );
		if ( hb.fp_val < hb.min )
			hb.fp_val = hb.min;
		else if ( hb.fp_val > hb.max )
			hb.fp_val = hb.max;
		memcpy( &data[hb.offset], &hb.fp_val, 4 * sizeof( unsigned char ) );
		printf( "::    Changed to %f.\n", hb.fp_val );
	}

	// Flush stdin if it overflowed
	if ( in_str[strlen( in_str )-1] != '\n' )
		while ( in_str[0] != '\n' && in_str[0] != EOF )
			fgets( in_str, 2, stdin );
	return( 0 );

}

struct handle
das_set_struct(	unsigned char* data, const char title[32], int ioffset, float imin, float imax ) {

	// Initialize struct with in values
	struct handle hb = {
		.hash = 0,
		.offset = ioffset,
		.fp_val = 0,
		.min = imin,
		.max = imax,
		.name = "(NULL)"
	};

	// Set title
	snprintf( hb.name, 32, "%s", title );

	// Set hash and float value
	int i = 0;
	unsigned char tbytes[4] = { 0, 0, 0, 0 };
	for ( i = 0; i < 4; i++ ) {
		hb.hash = ( hb.hash << 8 ) + data[hb.offset+i];
		tbytes[i] = data[hb.offset+i];
	}
	memcpy( &hb.fp_val, &tbytes, sizeof( float ) );

	// Return the new struct
	return( hb );

}

int das_file_export( const char* filename, struct handle* value, int num_values ) {

	// Were we passed something valid?
	if ( value == NULL )
		return( -1 );

	// Open file for write
	FILE * fp;
	if ( !( fp = fopen( filename, "wb" ) ) ) {
		fprintf( stderr, ":: ERROR: Cannot create file \"%s\"\n", filename );
		return( -1 );
	}

	// Write header, 12 bytes (12)
	unsigned char header[12] = { 'D', 'A', 'S', 'F', 'A', 'C', 'E', 'D', 'A', 'T', 'A', 0x0A };
	fwrite( header, sizeof( unsigned char ), 12, fp );

	// 4 bytes, number of values (5)
	unsigned char tbytes[4] = { 0, 0, 0, 0 };
	memcpy( &tbytes, &num_values, sizeof( unsigned char ) * 4 );
	fwrite( tbytes, sizeof( unsigned char ), 4, fp );
	fputc( 0x0A, fp );

	// Start of data (9*num_values)
	int i = 0;
	for ( i = 0; i < num_values; i++ ) {
		// 4 bytes index
		memcpy( &tbytes, &i, sizeof( unsigned char ) * 4 );
		fwrite( tbytes, sizeof( unsigned char ), 4, fp );
		// 4 bytes value
		memcpy( &tbytes, &value[i].fp_val, sizeof( unsigned char ) * 4 );
		fwrite( tbytes, sizeof( unsigned char ), 4, fp );
		// Seperator
		fputc( 0x0A, fp );
	}

	// Cleanup and return
	printf( ":: File saved to %s\n", filename );
	fclose( fp );
	return( 0 );

}

int das_import_file_write( const char* file, struct handle* hb, int num_values, unsigned char* data ) {

	// Check to see if file exists
	struct stat sb;
	if ( stat( file, &sb ) == -1 ) {
		fprintf( stderr, ":: ERROR: Cannot stat file %s.\n", file );
		return( -1 );
	}
	switch ( sb.st_mode & S_IFMT ) {
		case S_IFREG:
			break;
		default:
			fprintf( stderr, ":: ERROR: \"%s\" is not a file.\n", file );
			return( -1 );
	}

	// Don't proceed if filesize is incorrect
	if ( sb.st_size != ( num_values * 9 ) + 17 ) {
		fprintf( stderr, ":: ERROR: File size is off. Not a face data file.\n" );
		return( -1 );
	}

	// Read file into a memory buffer
	unsigned char* facedata = file_to_char( file, NULL );
	if ( facedata == NULL )
		return( -1 );

	// Check header of file
	unsigned char header[12] = { 'D', 'A', 'S', 'F', 'A', 'C', 'E', 'D', 'A', 'T', 'A', 0x0A };
	if ( memcmp( facedata, header, 12 ) != 0 ) {
		free( facedata );
		fprintf( stderr, ":: ERROR: Wrong header data.\n" );
		return( -1 );
	}

	// Copy bytes over
	int foffset = 21, i = 0;
	for ( i = 0; i < num_values; i++ ) {
		memcpy( &data[hb[i].offset], &facedata[foffset], 4 * sizeof( unsigned char ) );
		foffset += 9;
	}

	printf( ":: Imported data from %s\n", file );
	free( facedata );
	return 0;

}

int xml_mem_to_file( unsigned char* xmldata, int size, const char* filename ) {

	// Were we passed something valid?
	if ( xmldata == NULL )
		return( -1 );

	// Open file for write
	FILE * fp;
	if ( !( fp = fopen( filename, "wb" ) ) ) {
		fprintf( stderr, "  ERROR: Cannot create file \"%s\"\n", filename );
		return( -1 );
	}

	// Write data to file
	int i = 0;
	for ( i = 0; i < size; i++ ) {
		fputc( xmldata[i], fp );
		if ( xmldata[i] == '>' )
			fputc( '\n', fp );
	}

	// Cleanup and return
	fclose( fp );
	return( 0 );

}

int das_dump_xmls( unsigned char* data, int filesize ) {

	// Variables
	unsigned char xmlstr[5] = { 0x3C, 0x3F, 0x78, 0x6D, 0x6C };
	int shift_count = 0, xml_count = 0, i = 0, d = 0;
	struct xml_file xml = {
		.size = NULL, .offset = NULL, .shift_amount = NULL, .data = NULL
	};

	// Bit shift the file until the find a readable XML file
	// Loop 7 times, 1 bit shift to the right at a time
	for ( shift_count = 0; shift_count < 8; shift_count++ ) {
		// Scan 5 bytes at a time for "<?xml" until end of file
		for ( i = 0; i < filesize - 5; i++ ) {
			// We found an xml (probably)
			if ( memcmp( data + i, xmlstr, 5 ) == 0 ) {
				xml_count++;
				// Dynamically allocate memory
				xml.size =
					(int*)realloc( xml.size, xml_count * sizeof( int ) );
				xml.offset =
					(int*)realloc( xml.offset, xml_count * sizeof( int ) );
				xml.shift_amount =
					(int*)realloc( xml.shift_amount, xml_count * sizeof( int ) );
				xml.data =
					(unsigned char**)realloc( xml.data, xml_count * sizeof( unsigned char* ) );
				// Initalize values
				xml.size[xml_count-1] = 0;
				xml.offset[xml_count-1] = i;
				xml.shift_amount[xml_count-1] = shift_count;
				xml.data[xml_count-1] = NULL;
				// Get size of the xml from the 4 preceeding bytes (offset-4)
				for ( d = 0; d < 4; d++ )
					xml.size[xml_count-1] =
						( xml.size[xml_count-1] << 8 ) + data[xml.offset[xml_count-1]+d-4];
				// Allocate the xml data if its less than 1mb in size
				if ( xml.size[xml_count-1] <= 1000000 ) {
					xml.data[xml_count-1] =
						(unsigned char*)realloc( xml.data[xml_count-1],
							xml.size[xml_count-1] * sizeof( unsigned char ) );
					memcpy( xml.data[xml_count-1],
						data + xml.offset[xml_count-1],
						xml.size[xml_count-1] );
				}
				printf( "::  Found XML at %.8X (>>%.2d). Written to \"xml_file%.2d.xml\"\n",
					xml.offset[xml_count-1],
					xml.shift_amount[xml_count-1],
					xml_count );
			}
		}
		// Shift file 1 bit to the right
		unsigned char tbit = ( data[0] >> 1 ) | ( data[filesize-1] << 7 );
		for ( i = filesize - 1; i > 0; i-- )
			data[i] = ( data[i] >> 1 ) | ( data[i-1] << 7 );
		data[0] = tbit;
	}

	// Shift the bits back to the original
	unsigned char tbit = ( data[filesize - 1] << 8 ) | data[0];
	for ( i = 0; i < filesize - 1; i++ )
		data[i] = ( data[i] << 8 ) | data[i+1];
	data[filesize - 1] = tbit;

	// Write the xmls to file
	// The xml is unformatted, we will add newlines, but thats it for now.
	for ( i = 0; i < xml_count; i++ ) {
		char fname[16];
		snprintf( fname, 16, "xml_file%.2d.xml", i + 1 );
		xml_mem_to_file( xml.data[i], xml.size[i], fname );
	}

	// Cleanup and return
	int return_val = xml.shift_amount[0];
	for ( i = 0; i < xml_count; i++ )
		free( xml.data[i] );
	free( xml.size );
	free( xml.offset );
	free( xml.shift_amount );
	free( xml.data );
	return( return_val );

}