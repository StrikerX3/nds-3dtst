#include <cstdio>
#include <nds.h>
#include <fat.h>
#include <filesystem.h>

int main()
{
    fatInitDefault();

    videoSetMode(MODE_0_3D);
    vramSetBankA(VRAM_A_TEXTURE);
    vramSetBankB(VRAM_B_LCD);
	consoleDemoInit();

    REG_DISPCAPCNT = DCAP_MODE(DCAP_MODE_A)
                    | DCAP_SRC_A(DCAP_SRC_A_3DONLY)
                    | DCAP_SIZE(DCAP_SIZE_256x192)
                    | DCAP_OFFSET(0)
                    | DCAP_BANK(DCAP_BANK_VRAM_B);

    glInit();
    glViewport(0, 0, 255, 191);
    glClearColor(3, 3, 3, 31);
    glClearDepth(0x7FFF);
    glClearPolyID(63);
    glSetOutlineColor(0, 0x7E00);
    glEnable(GL_BLEND);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 3, 3, 0, -10, 10);

    glMatrixMode(GL_POSITION);
    glLoadIdentity();

    uint16_t texVertStripes[128 * 128];
    for (int y = 0; y < 128; y++)
        for (int x = 0; x < 128; x++)
            texVertStripes[y * 128 + x] = (x % 2) ? 0xFFFF : 0xABCD;

    int texVertStripesID;
    glGenTextures(1, &texVertStripesID);
    glBindTexture(0, texVertStripesID);
    glTexImage2D(0, 0, GL_RGB, TEXTURE_SIZE_128, TEXTURE_SIZE_128, 0, TEXGEN_TEXCOORD, texVertStripes);

    uint16_t texHorzStripes[128 * 128];
    for (int y = 0; y < 128; y++)
        for (int x = 0; x < 128; x++)
            texHorzStripes[y * 128 + x] = (y % 2) ? 0xFFFF : 0xABCD;

    int texHorzStripesID;
    glGenTextures(1, &texHorzStripesID);
    glBindTexture(0, texHorzStripesID);
    glTexImage2D(0, 0, GL_RGB, TEXTURE_SIZE_128, TEXTURE_SIZE_128, 0, TEXGEN_TEXCOORD, texHorzStripes);

    uint16_t texVertHorzStripes[128 * 128];
    for (int y = 0; y < 128; y++)
        for (int x = 0; x < 128; x++)
            texVertHorzStripes[y * 128 + x] = 0x8000
                | ((x % 2 && y % 2) ? 0x7FFF :
                   (x % 2) ? 0xABCD :
                   (y % 2) ? 0x765E :
                   0x7E29);

    int texVertHorzStripesID;
    glGenTextures(1, &texVertHorzStripesID);
    glBindTexture(0, texVertHorzStripesID);
    glTexImage2D(0, 0, GL_RGB, TEXTURE_SIZE_128, TEXTURE_SIZE_128, 0, TEXGEN_TEXCOORD, texVertHorzStripes);

    uint16_t texColorCoords[8 * 8];
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++)
            texColorCoords[y * 8 + x] = 0x8000
                | ((x << 2) | (x >> 1))
                | (((y << 2) | (y >> 1)) << 5)
                | (0x0F << 10);

    int texColorCoordsID;
    glGenTextures(1, &texColorCoordsID);
    glBindTexture(0, texColorCoordsID);
    glTexImage2D(0, 0, GL_RGB, TEXTURE_SIZE_8, TEXTURE_SIZE_8, 0, TEXGEN_TEXCOORD, texColorCoords);

    int x[4], y[4];
    x[0] =  64; y[0] =  32; x[1] =  64; y[1] = 160;
    x[2] = 192; y[2] = 160; x[3] = 192; y[3] =  32;
    int vertex = 0;
    bool antiAlias = false, edgeMark = false;
    int alpha = 31;
    bool wire = false;
    bool quad = true;
    int texMode = 0;
    unsigned int scrDumpCount = 0;
    bool dumpScreen = false;

    static constexpr const char *texModeNames[] = {
        "V stripes",
        "H stripes",
        "VH stripes",
        "Coord grid",
        "Color verts",
        "Plain white" };

    keysSetRepeat(20, 2);

    while (true)
    {
        printf("x1=%d, y1=%d, x2=%d, y2=%d\n", x[0], y[0], x[1], y[1]);
        printf("x3=%d, y3=%d, x4=%d, y4=%d\n", x[2], y[2], x[3], y[3]);
        printf("\n");
        printf("Current vertex: %d\n", vertex + 1);
        printf("\n");
        printf("[L+A]      Anti-aliasing: %s\n", antiAlias ? "on" : "off");
        printf("[L+B]      Edge marking: %s\n", edgeMark ? "on" : "off");
        printf("[L+Y/X]    Alpha: %d\n", alpha);
        printf("[L+Start]  Wireframe: %s\n", wire ? "on" : "off");
        printf("[L+Select] Shape: %s\n", quad ? "Quad" : "Triangle");
        printf("[L+\x1B/\x1A]    Texture: %s\n", texModeNames[texMode]);
        printf("\n");
        printf("ABXY to select vertex 1234\n");
        printf("Dpad to move selected vertex\n");
        printf("Hold ABXY for fine movement\n");
        printf("L+R to capture screen\n");
        if (scrDumpCount > 0) {
            printf("%d screen caps taken\n", scrDumpCount);
        }
        printf("\n");
        printf("Start+select to exit\n");

        scanKeys();
        uint16_t keys = keysHeld();
        uint16_t keysPressed = keysDown();
        uint16_t keysRepeat = keysDownRepeat();

        if (keys & KEY_L) {
            if (keysPressed & KEY_A)      antiAlias = !antiAlias;
            if (keysPressed & KEY_B)      edgeMark = !edgeMark;
            if (keysRepeat & KEY_X)       alpha = (alpha == 31) ? 31 : (alpha + 1);
            if (keysRepeat & KEY_Y)       alpha = (alpha == 0) ? 0 : (alpha - 1);
            if (keysPressed & KEY_LEFT)   texMode = (texMode + 5) % 6;
            if (keysPressed & KEY_RIGHT)  texMode = (texMode + 1) % 6;
            if (keysPressed & KEY_START)  wire = !wire;
            if (keysPressed & KEY_SELECT) quad = !quad;
            if (keysPressed & KEY_R)      dumpScreen = true;
        } else {
            if (keys & KEY_A) vertex = 0;
            if (keys & KEY_B) vertex = 1;
            if (keys & KEY_X) vertex = 2;
            if (keys & KEY_Y) vertex = 3;

            if (keys & (KEY_A | KEY_B | KEY_X | KEY_Y))
            {
                if (keysRepeat & KEY_UP)    y[vertex]--;
                if (keysRepeat & KEY_DOWN)  y[vertex]++;
                if (keysRepeat & KEY_LEFT)  x[vertex]--;
                if (keysRepeat & KEY_RIGHT) x[vertex]++;
            } else {
                if (keys & KEY_UP)    y[vertex]--;
                if (keys & KEY_DOWN)  y[vertex]++;
                if (keys & KEY_LEFT)  x[vertex]--;
                if (keys & KEY_RIGHT) x[vertex]++;
            }
        }

        if ((keys & KEY_START) && (keys & KEY_SELECT)) break;

        if (dumpScreen) {
            printf("\nCapturing screen...");
            swiWaitForVBlank();
            REG_DISPCAPCNT |= DCAP_ENABLE;
        }

        antiAlias ? glEnable(GL_ANTIALIAS) : glDisable(GL_ANTIALIAS);
        edgeMark ? glEnable(GL_OUTLINE) : glDisable(GL_OUTLINE);

        switch (texMode) {
            case 0:
                glEnable(GL_TEXTURE_2D);
                glBindTexture(0, texVertStripesID);
                break;
            case 1:
                glEnable(GL_TEXTURE_2D);
                glBindTexture(0, texHorzStripesID);
                break;
            case 2:
                glEnable(GL_TEXTURE_2D);
                glBindTexture(0, texVertHorzStripesID);
                break;
            case 3:
                glEnable(GL_TEXTURE_2D);
                glBindTexture(0, texColorCoordsID);
                break;
            default:
                glDisable(GL_TEXTURE_2D);
                break;
        }

        glPolyFmt(POLY_ALPHA(wire ? 0 : alpha) | POLY_CULL_NONE);

        quad ? glBegin(GL_QUAD) : glBegin(GL_TRIANGLE);
        glColor3b(255, 255, 255);

        switch (texMode) {
            case 0: GFX_TEX_COORD = TEXTURE_PACK(inttot16(0), inttot16(0)); break;
            case 1: GFX_TEX_COORD = TEXTURE_PACK(inttot16(0), inttot16(0)); break;
            case 2: GFX_TEX_COORD = TEXTURE_PACK(inttot16(0), inttot16(0)); break;
            case 3: GFX_TEX_COORD = TEXTURE_PACK(inttot16(0), inttot16(0)); break;
            case 4: glColor3b(255, 0, 0); break;
            case 5: glColor3b(255, 255, 255); break;
        }
        glVertex3v16(x[0] * 48, y[0] * 64, 0);

        switch (texMode) {
            case 0: GFX_TEX_COORD = GFX_TEX_COORD = TEXTURE_PACK(inttot16(0), inttot16(128)); break;
            case 1: GFX_TEX_COORD = GFX_TEX_COORD = TEXTURE_PACK(inttot16(0), inttot16(128)); break;
            case 2: GFX_TEX_COORD = GFX_TEX_COORD = TEXTURE_PACK(inttot16(0), inttot16(128)); break;
            case 3: GFX_TEX_COORD = GFX_TEX_COORD = TEXTURE_PACK(inttot16(0), inttot16(8)); break;
            case 4: glColor3b(255, 255, 0); break;
            case 5: glColor3b(255, 255, 255); break;
        }
        glVertex3v16(x[1] * 48, y[1] * 64, 0);

        switch (texMode) {
            case 0: GFX_TEX_COORD = GFX_TEX_COORD = TEXTURE_PACK(inttot16(128), inttot16(128)); break;
            case 1: GFX_TEX_COORD = GFX_TEX_COORD = TEXTURE_PACK(inttot16(128), inttot16(128)); break;
            case 2: GFX_TEX_COORD = GFX_TEX_COORD = TEXTURE_PACK(inttot16(128), inttot16(128)); break;
            case 3: GFX_TEX_COORD = GFX_TEX_COORD = TEXTURE_PACK(inttot16(8), inttot16(8)); break;
            case 4: glColor3b(0, 0, 255); break;
            case 5: glColor3b(255, 255, 255); break;
        }
        glVertex3v16(x[2] * 48, y[2] * 64, 0);

        if (quad) {
            switch (texMode) {
                case 0: GFX_TEX_COORD = GFX_TEX_COORD = TEXTURE_PACK(inttot16(128), inttot16(0)); break;
                case 1: GFX_TEX_COORD = GFX_TEX_COORD = TEXTURE_PACK(inttot16(128), inttot16(0)); break;
                case 2: GFX_TEX_COORD = GFX_TEX_COORD = TEXTURE_PACK(inttot16(128), inttot16(0)); break;
                case 3: GFX_TEX_COORD = GFX_TEX_COORD = TEXTURE_PACK(inttot16(8), inttot16(0)); break;
                case 4: glColor3b(0, 255, 0); break;
                case 5: glColor3b(255, 255, 255); break;
            }
            glVertex3v16(x[3] * 48, y[3] * 64, 0);
        }

        glEnd();
        glFlush(0);

        if (dumpScreen) {
            while(REG_DISPCAPCNT & DCAP_ENABLE) {}

            dumpScreen = false;
            char filename[26];
            sprintf(filename, "screencap-%d.bin", scrDumpCount);
            scrDumpCount++;

            static constexpr uint32_t magic = 0x43535344; // DSSC
            static constexpr uint16_t version = 1;
            static constexpr uint8_t reserved = 0x00;
            
            uint32_t params =
            /* byte 0 */   (antiAlias << 0) | (edgeMark << 1) | (wire << 2) | (quad << 3) | (texMode << 4)
            /* byte 1 */ | (alpha << 8);
            /* bytes 2 and 3 are reserved */

            FILE* file = fopen(filename, "wb");
            fseek(file, 0, SEEK_SET);

            /* 0x00 */ fwrite(&magic, 1, sizeof(magic), file);
            
            /* 0x04 */ fwrite(&version, 1, sizeof(version), file);
            /* 0x06 */ fwrite(&reserved, 1, sizeof(reserved), file);
            /* 0x07 */ fwrite(&reserved, 1, sizeof(reserved), file);
            
            /* 0x08 */ fwrite(&params, 1, sizeof(params), file);

            /* 0x0C */ fwrite(&reserved, 1, sizeof(reserved), file);
            /* 0x0D */ fwrite(&reserved, 1, sizeof(reserved), file);
            /* 0x0E */ fwrite(&reserved, 1, sizeof(reserved), file);
            /* 0x0F */ fwrite(&reserved, 1, sizeof(reserved), file);

            /* 0x10..0x3F */
            for (size_t i = 0; i < 4; i++) {
                fwrite(&x[i], 1, sizeof(x[i]), file);
                fwrite(&y[i], 1, sizeof(y[i]), file);
                fwrite(&reserved, 4, sizeof(reserved), file); // reserved for Z
            }
            
            /* 0x40 - VRAM dump */
            fwrite(VRAM_B, 256*192, sizeof(uint16_t), file);

            fclose(file);
        }

        swiWaitForVBlank();
        consoleClear();
    }

    return 0;
}
