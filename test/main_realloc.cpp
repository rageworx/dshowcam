#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>

#include <dshowcam.h>

void prtlist( DShowCamera*& p )
{
    if ( p == nullptr )
    {
        p = new DShowCamera;
    }

    if ( p != nullptr )
    {
        DShowCamera::DeviceInfos dl;

        p->EnermateDevice( &dl );

        printf( "Total %zu device(s) found.\n", dl.size() );

        for( size_t cnt=0; cnt<dl.size(); cnt++ )
        {
            printf( "%3zu: %S [%S]\n",
                    cnt,
                    dl[cnt].name.c_str(), 
                    dl[cnt].path.c_str() );
        }

        fflush( stdout );
    }
}

void capture( DShowCamera*& p, size_t idx )
{
    if ( p != nullptr )
    {
        delete p;
    }

    p = new DShowCamera;

    if ( p != nullptr )
    {
        if ( p->SelectDevice( idx ) == false )
        {
            printf( "Error: Cannot select device %zu.\n", idx );
            return;
        }

        if ( p->SelectConfig( 0 ) == false )
        {
            printf( "Error: Cannot select default configuration.\n" );
            return;
        }

        if ( p->StartPoll() == false )
        {
            printf( "Error: failed to start polling !\n" );
            return;
        }

        uint8_t* buff = nullptr;
        size_t   bufflen = 0;

        if ( p->GrabAFrame( buff, bufflen ) == false )
        {
            printf( "Error: failed to grab a frame !\n" );
            return;
        }

        printf( "Grab frame : %p (%zu bytes)\n", buff, bufflen );

        delete[] buff;
        buff = nullptr;

        if ( p->StopPoll() == false )
        {
            printf( "Error: failed to stop pollong !\n" );
            return;
        }

        delete p;
        p = nullptr;
    }
}

int main( int argc, char** argv )
{
    DShowCamera::InitInstance();

    DShowCamera* dsc = nullptr;
    size_t       idx = 0;

    printf("DShowCamera testing for minGW-W64.\n" );
    fflush( stdout );

    while( true )
    {
        printf( "Input command :" );
        fflush( stdout );

        char cmd[128] = {0};

        if ( fgets( cmd, sizeof(cmd), stdin ) != NULL )
        {
            if ( strncmp( cmd, "list", 4 ) == 0 )
            {
                prtlist( dsc );
            }
            else
            if ( strncmp( cmd, "capture", 6 ) == 0 ||
                 strncmp( cmd, "cap", 3 ) == 0 )
            {
                capture( dsc, idx );
            }
            else
            if ( strncmp( cmd, "select", 6 ) == 0 )
            {
                if ( strlen( cmd ) > 8 )
                {
                    int ix = atoi( &cmd[7] );
                    if ( ix > 0 )
                    {
                        idx = ix;
                        printf( "Camera selected: %zu\n", idx );
                        fflush( stdout );
                    }
                }
                else
                {
                    printf( "Nothing to selected.\n" );
                    fflush( stdout );
                }
            }
            else
            if ( strncmp( cmd, "sel", 3 ) == 0 )
            {
                if ( strlen( cmd ) > 5 )
                {
                    int ix = atoi( &cmd[4] );
                    if ( ix > 0 )
                    {
                        idx = ix;
                        printf( "Camera selected: %zu\n", idx );
                        fflush( stdout );
                    }
                }
            }
            else
            if ( strncmp( cmd, "quit", 4 ) == 0 || 
                 strncmp( cmd, "q" , 1 ) == 0 )
            {
                break;
            }
            else
            {
                printf( "unknown command.\n" );
                fflush( stdout );
            }
        }
    }

    if ( dsc != NULL )
    {
        delete dsc;
        dsc = NULL;
    }

    DShowCamera::ReleaseInstance();

    return 0;
}
