#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define SIZE 1024

// server functions
void start_server( int port, const char* directory );
void recive_file( int sockfd, const char* directory );

// client functions
void start_client();
void send_file( const char* filename, int sockfd );

// utilities
void get_filename( char* path );
int string_length( const char* string );
void get_absolute_path( char* buffer, const char* path, const char* filename );

int main( int argc, char** argv )
{
    if ( argc - 1 > 1 && argc < 6 )
    {
        if ( !strcmp( argv[1], "server" ) )
        {
            int port = 8080; // 8080 is base port

            // set port if specified
            if ( argc - 1 > 2 )
            {
                int port_ = strtol( argv[3], NULL, 10 );

                // check if port number is correct
                if ( port_ < 65536 && port_ > 0 )
                    port = port_;
                else // port number is too big or too small
                {
                    printf( "%d IS NOT A VALID PORT NUMBER\n", port_ );
                    exit( 1 );
                }
            }
            while ( 1 )
                start_server( port, argv[2] );
        }
        else if ( !strcmp( argv[1], "client" ) && argc - 1 > 2 )
        {
            int port = 8080; // 8080 is base port

            // set port if specified
            if ( argc - 1 > 3 )
            {
                int port_ = strtol( argv[4], NULL, 10 );

                // check if port number is correct
                if ( port_ < 65536 && port_ > 0 )
                    port = port_;
                else // port number is too big or too small
                {
                    printf( "%d IS NOT A VALID PORT NUMBER\n", port_ );
                    exit( 1 );
                }
            }
            start_client( argv[2], argv[3], port );
        }
    }
    else
        printf( "Usage:\nSERVER:\nfile_transfer server <reciving_directory> (port)\nCLIENT\nfile_transfer client <filename> <ip> (port)\n" );

    return 0;
}

void start_server( int port, const char* directory )
{
    char *ip = "127.0.0.1";
    int option = 1;
    int e;

    int sockfd, new_sock;
    struct sockaddr_in server_addr, new_addr;
    socklen_t addr_size;

    // start server
    sockfd = socket( AF_INET, SOCK_STREAM, 0 );
    if( sockfd < 0 )
    {
        perror( "[-]Error in socket" );
        exit( 1 );
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = port;
    server_addr.sin_addr.s_addr = inet_addr( ip );

    e = bind( sockfd, ( struct sockaddr* )&server_addr, sizeof( server_addr ) );
    if( e < 0 )
    {
        perror( "[-]Error in Binding" );
        exit( 1 );
    }

    e = listen( sockfd, 10 );
    if( e != 0 )
    {
        perror( "[-]Error in Binding" );
        exit( 1 );
    }

    addr_size = sizeof( new_addr );
    new_sock = accept( sockfd, ( struct sockaddr* )&new_addr, &addr_size );

    // recive the file
    recive_file( new_sock, directory );
    
    // close connections
    close( sockfd );
    close( new_sock );

    return;
}

void recive_file( int sockfd, const char* directory )
{
    int is_good = 0;
    int count = 0;

    // allocate buffer for trasmitted datas
    char* buffer = NULL;
    buffer = (char*)malloc( SIZE * sizeof( char ) );
    if ( buffer == NULL ) // heap is not allocated
    {
        printf( "Not enough memory!\n" );
        exit( 1 );
    }
    bzero( buffer, SIZE ); // clear buffer

    FILE* file = NULL;

    printf( "Reciving datas\n" );

    // repeat until has bytes to send
    while ( 1 )
    {
        is_good = recv( sockfd, buffer, SIZE, 0 );
        if ( is_good <= 0 )
            break;

        // first recived is filename
        if ( count == 0 )
        {
            char* temp_buffer = (char*)malloc( SIZE * sizeof( char ) );
            if ( temp_buffer == NULL ) // heap is not allocated
            {
                printf( "Not enough memory!\n" );
                exit( 1 );
            }

            // make file absolute path
            get_filename( buffer );
            get_absolute_path( temp_buffer, directory, buffer );
            printf( "'%s'\n", temp_buffer );
            
            // open file
            file = fopen( temp_buffer, "w" );
            
            count += 1;

            // free heap
            free( temp_buffer );
        }
        else // write in file
            fprintf( file, "%s", buffer );

        bzero( buffer, SIZE ); // clear buffer

    }

    printf( "Datas recived\n" );

    // close file
    fclose( file );

    // free heap
    free( buffer );

    return;
}

void start_client( const char* filename, const char* ip, int port )
{
    int sockfd;
    int e;
    struct sockaddr_in server_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // start client
    if( sockfd < 0 )
    {
        perror( "[-]Error in socket" );
        exit( 1 );
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = port;
    server_addr.sin_addr.s_addr = inet_addr( ip );

    e = connect( sockfd, ( struct sockaddr* )&server_addr, sizeof( server_addr ) );
    if( e == -1 )
    {
        perror( "[-]Error in Connecting" );
        exit( 1 );
    }

    // send file
    send_file( filename, sockfd );
    printf( "File sent successfully.\n" );
    
    //close connetion
    close( sockfd );

    return;
}

void send_file( const char* filename, int sockfd )
{
    int is_good = 0;

    // allocate buffer for trasmitted datas
    char* buffer = NULL;
    buffer = (char*)malloc( SIZE * sizeof( char ) );
    if ( buffer == NULL ) // heap is not allocated
    {
        printf( "Not enough memory!\n" );
        exit( 1 );
    }
    bzero( buffer, SIZE ); // clear buffer

    // file opening
    FILE* file = fopen( filename, "r" );
    // check if file exists
    if ( !file )
    {
        perror( "[-]Error in reading file." );
        exit( 1 );
    }

    // Send filename
    if( send( sockfd, filename, SIZE, 0 ) == -1 )
        exit( 1 );

    // Send file
    while( fgets( buffer, SIZE, file ) != NULL )
    {
        is_good = send( sockfd, buffer, SIZE, 0 );
        if ( is_good == -1 )
            exit( 1 );

        bzero( buffer, SIZE ); // clear buffer
    }

    // free heap
    free( buffer );

    return;
}

void get_filename( char* path )
{
    int length = string_length( path );
    int last_slash = -1;

    // temporary variable to hold filename
    char* filename = (char*)malloc( length * sizeof( char ) );
    if ( filename == NULL ) // heap is not allocated
    {
        printf( "Not enough memory!\n" );
        exit( 1 );
    }

    // find last slash
    for ( int i = 0; i < length + 1; i++ )
        if ( path[i] == '/' )
            last_slash = i;

    // from path gets only filename
    if ( last_slash != -1 ) // file in local directory
        for ( int i = 0; i < length + 1; i++ )
            filename[i] = path[ last_slash + i + 1 ];
    else // absolute path
        for ( int i = 0; i < length + 1; i++ )
            filename[i] = path[ last_slash + i ];

    // clear path
    bzero( path, length );

    // write filename in original path
    length = string_length( filename );
    for ( int i = 0; i < length + 1; i++ )
        path[i] = filename[i];

    // free heap
    free( filename );

    return;
}

int string_length( const char* string )
{
    int i = 0;
    while ( string[i] != '\0' )
        i++;
    return i;
}

void get_absolute_path( char* buffer, const char* path, const char* filename )
{
    int path_length = string_length( path );
    int filename_length = string_length( filename );

    // append path
    for ( int i = 0; i < path_length; i++ )
        buffer[i] = path[i];

    // if character of path is not a / appends it
    if ( buffer[ path_length - 1 ] != '/' )
    {
        buffer[ path_length ] = '/';
        path_length += 1;
    }

    // append filename
    for ( int i = path_length; i < path_length + filename_length + 2; i++ )
        buffer[i] = filename[ i - path_length ];
}

