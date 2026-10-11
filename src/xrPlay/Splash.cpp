#include "stdafx.h"
#ifdef IXR_WINDOWS
#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>
#include <SDL_Ext/SDL_image.h>
#include "../xrCore/git_version.h"
#include <cstdlib>
#include <ctime>

#include "Splash.h"

#include <Windows.h>
#include<string>
#ifdef _EDITOR
    #include "../Editors/xrECore/resource.h"
#else
    #include "resource.h"
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define HAVE_SLD3TTF 0

//#if HAVE_SLD3TTF
//#include <SDL3_ttf/SDL_ttf.h>
//#endif

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#pragma warning(disable: 4047)
HINSTANCE hInstanceG = (HINSTANCE)&__ImageBase;
#pragma warning(default: 4047)

namespace splash
{
    // == WINDOW
    int WINDOW_WIDTH = 98;
    int WINDOW_HEIGHT = 78;

    static SDL_Window* window = nullptr;
    static SDL_Renderer* renderer = nullptr;

    const char* SPLASH_STATUS = "";
    int progress_percent = 0;
    int background_resource_id = 0;

    // WINDOW ==

    // == FONT
#if HAVESDL3TTF
#else
    SDL_Texture* fontTexture = nullptr;

    const int CHARS_PER_ROW = 14;
    const int CHARS_PER_COL = 7;
    //bool outText = false;

    int CHAR_WIDTH = 0;
    int CHAR_HEIGHT = 0;
#endif

    SPLASH_API void SetProgressStatus(int prog, const char* status)
    {
        progress_percent = prog;
        SPLASH_STATUS = status;
    }

    SDL_Surface* LoadPNGSurfaceFromResource(unsigned char* imageData, LPCTSTR lpName, LPCTSTR lpType) {
        HMODULE hMODULE = hInstanceG;

        HRSRC hRes = FindResource(hMODULE, lpName, lpType);
        if (!hRes) {
            //ErrorMsg("Failed to find resource (ID %d)", lpName);
            return nullptr;
        }

        HGLOBAL hMem = LoadResource(hInstanceG, hRes);
        if (!hMem) {
            //ErrorMsg("Failed to load resource (ID %d)", lpName);
            return nullptr;
        }

        void* pResData = LockResource(hMem);
        if (!pResData) {
            //ErrorMsg("Failed to lock resource (ID %d)", lpName);
            return nullptr;
        }

        DWORD resSize = SizeofResource(hInstanceG, hRes);

        int width, height, channels;
        imageData = stbi_load_from_memory((unsigned char*)pResData, resSize, &width, &height, &channels, STBI_rgb_alpha);
        if (!imageData) {
            //ErrorMsg("Failed to decode PNG (ID %d)", lpName);
            return nullptr;
        }

        SDL_Surface* surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32, imageData, width * 4);
        if (!surface) {
            stbi_image_free(imageData);
            //ErrorMsg("Failed to create pixel format (ID %d). %s", lpName, SDL_GetError());
            return nullptr;
        }

        return surface;
    }

    void RenderText(const char* text, int x, int y) {
        if (!fontTexture) return;

        size_t len = strlen(text);
        for (size_t i = 0; i < len; ++i) {
            unsigned char c = static_cast<unsigned char>(text[i]);
            int charIndex = c - 32;
            int srcX = (charIndex % CHARS_PER_ROW) * CHAR_WIDTH;
            int srcY = (charIndex / CHARS_PER_ROW) * CHAR_HEIGHT;

            SDL_FRect srcRect = { (float)srcX, (float)srcY, (float)CHAR_WIDTH, (float)CHAR_HEIGHT };
            SDL_FRect dstRect = { (float)x + i * CHAR_WIDTH, (float)y, (float)CHAR_WIDTH, (float)CHAR_HEIGHT };

            SDL_RenderTexture(renderer, fontTexture, &srcRect, &dstRect);
        }
    }
    void UpdatepProgressBar(int progress = 0, const char* status = "")
    {
        int pgHeight = 10;

        SDL_FRect progressBarBackground = { 0, (float)WINDOW_HEIGHT - (float)pgHeight, (float)WINDOW_WIDTH, (float)pgHeight };
        SDL_FRect progressBarFill =
        {
            progressBarBackground.x,
            progressBarBackground.y,
            (progress * progressBarBackground.w) / 100,
            progressBarBackground.h
        };

        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderFillRect(renderer, &progressBarBackground);

        SDL_SetRenderDrawColor(renderer, 58, 134, 255, 255);
        SDL_RenderFillRect(renderer, &progressBarFill);

        //RenderText(status, (WINDOW_WIDTH - (strlen(status) * CHAR_WIDTH)) / 2, progressBarBackground.y - CHAR_HEIGHT);
        RenderText(status, 32.f, progressBarBackground.y - CHAR_HEIGHT - (CHAR_HEIGHT / 3));
    }

#ifdef _EDITOR
    SPLASH_API void SetBackground(int id)
    {
        background_resource_id = id;
        return;
    }
#endif

    bool running = true;
	SPLASH_API void Show()
	{
		srand((unsigned)time(nullptr));

		SDL_SetAppMetadata("Ultimate X-Ray", _VER, "com.uxray-team.ultimate-xray");

		if (!SDL_Init(SDL_INIT_VIDEO))
		{
			return;
		}

		unsigned char* imageData = nullptr;

		SDL_Surface* surface = nullptr;

		// check if exist splash.png in exe dir
		{
			bool extern_splash = false;

	// \/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
	// Mr Forserx, dobav' SDL3_image. Bez nego ne work load png from bin dir
	// /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ .
#if 0

            std::wstring exePath;
            exePath.resize(MAX_PATH, 0);
            auto path_size(GetModuleFileNameW(nullptr, &exePath.front(), MAX_PATH));
            exePath.resize(path_size);

            if (path_size != 0 && path_size < MAX_PATH)
            {
                size_t lastSlash = exePath.find_last_of('\\');

                if (lastSlash != -1)
                    exePath.replace(lastSlash + 1, exePath.size() - (lastSlash + 1), L"");

                exePath += L"splash.png";

                if (GetFileAttributesW(exePath.c_str()) != INVALID_FILE_ATTRIBUTES)
                {
                    //blin blyat, i hope this works with unicode paths
                    int size = WideCharToMultiByte(
                        CP_UTF8,
                        0,
                        exePath.c_str(),
                        -1,
                        nullptr,
                        0,
                        nullptr,
                        nullptr
                    );
                    std::string result(size - 1, '\0');

                    WideCharToMultiByte(
                        CP_UTF8,
                        0,
                        exePath.c_str(),
                        -1,
                        &result[0],
                        size,
                        nullptr,
                        nullptr
                    );

                    surface = IMG_Load(result.c_str());
                    extern_splash = (surface != nullptr);
                }
            }

#endif

			if (!extern_splash)
			{
#ifndef _EDITOR
				CEngineExternal engineExternal; // Hack
				auto platform = engineExternal.GetCurrentPlatform();

				if (platform == EEngineExternalPlatform::ShadowOfChernobyl)
				{
					background_resource_id = IDB_SOC_SPLASH_BG;
				}
				else if (platform == EEngineExternalPlatform::ClearSky)
				{
					background_resource_id = IDB_CS_SPLASH_BG;
				}
				else
				{
					background_resource_id = IDB_COP_SPLASH_BG;
				}
#endif
				surface = LoadPNGSurfaceFromResource(imageData, MAKEINTRESOURCE(background_resource_id), TEXT("PNG"));

				if (!surface)
				{
					SDL_Log("Couldn't load bitmap: %s", SDL_GetError());
					return;
				}
			}
		}
		WINDOW_WIDTH = surface->w;
		WINDOW_HEIGHT = surface->h;

        u32 window_flags = SDL_WINDOW_BORDERLESS | SDL_WINDOW_NOT_FOCUSABLE;
#ifdef IXR_WINDOWS
        if (!IsDebuggerPresent())
#endif
        {
			window_flags |= SDL_WINDOW_ALWAYS_ON_TOP;
        }
		if (!SDL_CreateWindowAndRenderer("Ultimate X-Ray", WINDOW_WIDTH, WINDOW_HEIGHT, window_flags, &window, &renderer))
		{
			SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
			return;
		}
		SDL_SetRenderLogicalPresentation(renderer, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);

		//////////////////////////////
		SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
		if (!texture)
		{
			SDL_Log("Couldn't create static texture: %s", SDL_GetError());
			return;
		}
		SDL_DestroySurface(surface);

		// overlay surface (noise + scanlines)
		SDL_Surface* overlaySurf = SDL_CreateSurface(WINDOW_WIDTH, WINDOW_HEIGHT, SDL_PIXELFORMAT_RGBA8888);
		//
		SDL_Texture* overlayTex = SDL_CreateTexture(
			renderer,
			SDL_PIXELFORMAT_RGBA8888,
			SDL_TEXTUREACCESS_STREAMING,
			WINDOW_WIDTH,
			WINDOW_HEIGHT
		);
		//
		SDL_SetTextureBlendMode(overlayTex, SDL_BLENDMODE_BLEND);
		//////////////////////////////

		// font
		surface = LoadPNGSurfaceFromResource(imageData, MAKEINTRESOURCE(IDB_FONT), TEXT("PNG"));
		stbi_image_free(imageData);

		fontTexture = SDL_CreateTextureFromSurface(renderer, surface);

		CHAR_WIDTH = surface->w / CHARS_PER_ROW;
		CHAR_HEIGHT = surface->h / CHARS_PER_COL;

		Uint64 prevTicks = SDL_GetTicks();

		SDL_PropertiesID props = SDL_GetWindowProperties(window);
		HWND hwnd = (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);

		if (!hwnd)
		{
			SDL_Log("Failed to get HWND: %s", SDL_GetError());
		}
		else
		{
			SetForegroundWindow(hwnd);
		}

		while (running)
		{
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
			SDL_RenderClear(renderer);

			SDL_RenderTexture(renderer, texture, nullptr, nullptr);

			UpdatepProgressBar(progress_percent, SPLASH_STATUS);

#ifndef _EDITOR
#ifdef DEBUG_DRAW
#ifdef NDEBUG
			RenderText("DEV BUILD", 0, 0);
#else
			RenderText("DEBUG BUILD", 0, 0);
#endif
#endif // !_NDEBUG
#endif // _EDITOR

			SDL_RenderPresent(renderer);

			SDL_Delay(16); // ~60 FPS
		}

		SDL_DestroyTexture(texture);
		SDL_DestroyTexture(overlayTex);
		SDL_DestroySurface(overlaySurf);
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
	}

    SPLASH_API void Close()
    {
        running = false;
    }
}
#else
namespace splash
{
    void SetBackground(int)
    {

    }
    int Show()
    {
        return 1;
    }
    void Close()
    {

    }
    void SetProgressStatus(int prog, const char* status)
    {

    }
}

#endif