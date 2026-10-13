/* compile: cc win_player.c -o win_player.exe -L<path/libheimdall> -lheimdall */

#include <windows.h>
#include <assert.h>

#include <heimdall/heimdall.h>

#define SCREEN_WIDTH  1500
#define SCREEN_HEIGHT  900

struct heimdall_context
{
    libheimdall_instance_t     *p_libheimdall;
    libheimdall_media_player_t *p_mediaplayer;
};


static const char *AspectRatio = NULL;

static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if( message == WM_CREATE )
    {
        /* Store p_mediaplayer for future use */
        CREATESTRUCT *c = (CREATESTRUCT *)lParam;
        SetWindowLongPtr( hWnd, GWLP_USERDATA, (LONG_PTR)c->lpCreateParams );
        return 0;
    }

    LONG_PTR p_user_data = GetWindowLongPtr( hWnd, GWLP_USERDATA );
    if( p_user_data == 0 )
        return DefWindowProc(hWnd, message, wParam, lParam);
    struct heimdall_context *ctx = (struct heimdall_context *)p_user_data;

    switch(message)
    {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_DROPFILES:
            {
                HDROP hDrop = (HDROP)wParam;
                char file_path[MAX_PATH];
                libheimdall_media_player_stop_async( ctx->p_mediaplayer );

                if (DragQueryFile(hDrop, 0, file_path, sizeof(file_path)))
                {
                    libheimdall_media_t *p_media = libheimdall_media_new_path( file_path );
                    libheimdall_media_t *p_old_media = libheimdall_media_player_get_media( ctx->p_mediaplayer );
                    libheimdall_media_player_set_media( ctx->p_mediaplayer, p_media );
                    libheimdall_media_release( p_old_media );

                    libheimdall_media_player_play( ctx->p_mediaplayer );
                }
                DragFinish(hDrop);
            }
            return 0;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            {
                int key = tolower( MapVirtualKey( (UINT)wParam, 2 ) );
                if (key == 'a')
                {
                    if (AspectRatio == NULL)
                        AspectRatio = "16:10";
                    else if (strcmp(AspectRatio,"16:10")==0)
                        AspectRatio = "16:9";
                    else if (strcmp(AspectRatio,"16:9")==0)
                        AspectRatio = "4:3";
                    else if (strcmp(AspectRatio,"4:3")==0)
                        AspectRatio = "185:100";
                    else if (strcmp(AspectRatio,"185:100")==0)
                        AspectRatio = "221:100";
                    else if (strcmp(AspectRatio,"221:100")==0)
                        AspectRatio = "235:100";
                    else if (strcmp(AspectRatio,"235:100")==0)
                        AspectRatio = "239:100";
                    else if (strcmp(AspectRatio,"239:100")==0)
                        AspectRatio = "5:3";
                    else if (strcmp(AspectRatio,"5:3")==0)
                        AspectRatio = "5:4";
                    else if (strcmp(AspectRatio,"5:4")==0)
                        AspectRatio = "1:1";
                    else if (strcmp(AspectRatio,"1:1")==0)
                        AspectRatio = "fill";
                    else if (strcmp(AspectRatio,"fill")==0)
                        AspectRatio = NULL;
                    libheimdall_video_set_aspect_ratio( ctx->p_mediaplayer, AspectRatio );
                }
                break;
            }
        default: break;
    }

    return DefWindowProc (hWnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    WNDCLASSEX wc;
    char *file_path;
    struct heimdall_context Context;
    libheimdall_media_t *p_media;
    (void)hPrevInstance;
    HWND hWnd;

    /* remove "" around the given path */
    if (lpCmdLine[0] == '"')
    {
        file_path = _strdup( lpCmdLine+1 );
        if (file_path[strlen(file_path)-1] == '"')
            file_path[strlen(file_path)-1] = '\0';
    }
    else
        file_path = _strdup( lpCmdLine );

    Context.p_libheimdall = libheimdall_new( 0, NULL );
    p_media = libheimdall_media_new_path( file_path );
    free( file_path );
    Context.p_mediaplayer = libheimdall_media_player_new_from_media(
                                                   Context.p_libheimdall, p_media,
                                                   NULL, NULL );

    ZeroMemory(&wc, sizeof(WNDCLASSEX));

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = TEXT("WindowClass");

    RegisterClassEx(&wc);

    RECT wr = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    hWnd = CreateWindowEx(0,
                          TEXT("WindowClass"),
                          TEXT("libheimdall Demo app"),
                          WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                          CW_USEDEFAULT, CW_USEDEFAULT,
                          wr.right - wr.left,
                          wr.bottom - wr.top,
                          NULL,
                          NULL,
                          hInstance,
                          &Context);
    DragAcceptFiles(hWnd, TRUE);

    libheimdall_media_player_set_hwnd(Context.p_mediaplayer, hWnd);

    ShowWindow(hWnd, nCmdShow);

    libheimdall_media_player_play( Context.p_mediaplayer );

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        if(msg.message == WM_QUIT)
            break;
    }

    libheimdall_media_player_stop_async( Context.p_mediaplayer );

    libheimdall_media_release( libheimdall_media_player_get_media( Context.p_mediaplayer ) );
    libheimdall_media_player_release( Context.p_mediaplayer );
    libheimdall_release( Context.p_libheimdall );

    return (int)msg.wParam;
}
