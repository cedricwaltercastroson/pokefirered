#include "global.h"
#include "gflib.h"
#include "task.h"
#include "wild_encounter.h"

static void Task_SmoothBlendLayers(u8 taskId);

static const u8 sMonSpriteAnchorCoords[][5] = {
    [SPECIES_BULBASAUR       - 1] = {0x16, 0x1b, 0x30, 0x16, 0x29},
    [SPECIES_IVYSAUR         - 1] = {0x14, 0x1b, 0x30, 0x15, 0x2a},
    [SPECIES_VENUSAUR        - 1] = {0x1b, 0x20, 0x20, 0x1b, 0x33},
    [SPECIES_CHARMANDER      - 1] = {0x14, 0x0f, 0x38, 0x13, 0x1e},
    [SPECIES_CHARMELEON      - 1] = {0x0d, 0x11, 0x30, 0x0f, 0x22},
    [SPECIES_CHARIZARD       - 1] = {0x1b, 0x07, 0x28, 0x1a, 0x19},
    [SPECIES_SQUIRTLE        - 1] = {0x19, 0x13, 0x30, 0x1a, 0x22},
    [SPECIES_WARTORTLE       - 1] = {0x16, 0x13, 0x30, 0x17, 0x21},
    [SPECIES_BLASTOISE       - 1] = {0x12, 0x08, 0x28, 0x15, 0x19},
    [SPECIES_CATERPIE        - 1] = {0x15, 0x14, 0x30, 0x13, 0x24},
    [SPECIES_METAPOD         - 1] = {0x13, 0x1d, 0x30, 0x18, 0x2a},
    [SPECIES_BUTTERFREE      - 1] = {0x12, 0x1b, 0x08, 0x12, 0x2a},
    [SPECIES_WEEDLE          - 1] = {0x16, 0x1c, 0x30, 0x15, 0x2b},
    [SPECIES_KAKUNA          - 1] = {0x19, 0x13, 0x30, 0x1a, 0x24},
    [SPECIES_BEEDRILL        - 1] = {0x19, 0x16, 0x08, 0x19, 0x25},
    [SPECIES_PIDGEY          - 1] = {0x15, 0x12, 0x30, 0x12, 0x1e},
    [SPECIES_PIDGEOTTO       - 1] = {0x24, 0x0e, 0x30, 0x1f, 0x1e},
    [SPECIES_PIDGEOT         - 1] = {0x0c, 0x13, 0x08, 0x0a, 0x23},
    [SPECIES_RATTATA         - 1] = {0x17, 0x1e, 0x30, 0x18, 0x2d},
    [SPECIES_RATICATE        - 1] = {0x12, 0x14, 0x30, 0x10, 0x26},
    [SPECIES_SPEAROW         - 1] = {0x15, 0x14, 0x30, 0x15, 0x1f},
    [SPECIES_FEAROW          - 1] = {0x0c, 0x27, 0x00, 0x0b, 0x3f},
    [SPECIES_EKANS           - 1] = {0x11, 0x0f, 0x30, 0x14, 0x1d},
    [SPECIES_ARBOK           - 1] = {0x1b, 0x01, 0x28, 0x1d, 0x10},
    [SPECIES_PIKACHU         - 1] = {0x19, 0x13, 0x30, 0x1a, 0x1f},
    [SPECIES_RAICHU          - 1] = {0x19, 0x14, 0x28, 0x1c, 0x22},
    [SPECIES_SANDSHREW       - 1] = {0x17, 0x16, 0x30, 0x18, 0x25},
    [SPECIES_SANDSLASH       - 1] = {0x11, 0x13, 0x28, 0x13, 0x22},
    [SPECIES_NIDORAN_F       - 1] = {0x16, 0x15, 0x30, 0x18, 0x21},
    [SPECIES_NIDORINA        - 1] = {0x1f, 0x17, 0x30, 0x1e, 0x28},
    [SPECIES_NIDOQUEEN       - 1] = {0x10, 0x0b, 0x28, 0x13, 0x1c},
    [SPECIES_NIDORAN_M       - 1] = {0x15, 0x22, 0x28, 0x15, 0x31},
    [SPECIES_NIDORINO        - 1] = {0x13, 0x1e, 0x28, 0x1b, 0x2d},
    [SPECIES_NIDOKING        - 1] = {0x12, 0x15, 0x28, 0x13, 0x27},
    [SPECIES_CLEFAIRY        - 1] = {0x19, 0x14, 0x30, 0x1b, 0x24},
    [SPECIES_CLEFABLE        - 1] = {0x1c, 0x12, 0x30, 0x1d, 0x21},
    [SPECIES_VULPIX          - 1] = {0x10, 0x16, 0x30, 0x0e, 0x25},
    [SPECIES_NINETALES       - 1] = {0x28, 0x10, 0x28, 0x27, 0x1e},
    [SPECIES_JIGGLYPUFF      - 1] = {0x1d, 0x15, 0x30, 0x1e, 0x25},
    [SPECIES_WIGGLYTUFF      - 1] = {0x1a, 0x12, 0x30, 0x1c, 0x22},
    [SPECIES_ZUBAT           - 1] = {0x14, 0x1d, 0x08, 0x14, 0x29},
    [SPECIES_GOLBAT          - 1] = {0x23, 0x1a, 0x00, 0x21, 0x2e},
    [SPECIES_ODDISH          - 1] = {0x1d, 0x20, 0x28, 0x1d, 0x2b},
    [SPECIES_GLOOM           - 1] = {0x1c, 0x18, 0x28, 0x1c, 0x27},
    [SPECIES_VILEPLUME       - 1] = {0x1c, 0x1e, 0x28, 0x1d, 0x2a},
    [SPECIES_PARAS           - 1] = {0x1d, 0x1a, 0x30, 0x1c, 0x27},
    [SPECIES_PARASECT        - 1] = {0x1e, 0x1c, 0x28, 0x20, 0x2e},
    [SPECIES_VENONAT         - 1] = {0x1b, 0x16, 0x10, 0x1a, 0x29},
    [SPECIES_VENOMOTH        - 1] = {0x12, 0x19, 0x08, 0x16, 0x27},
    [SPECIES_DIGLETT         - 1] = {0x1e, 0x16, 0x30, 0x1c, 0x23},
    [SPECIES_DUGTRIO         - 1] = {0x1a, 0x11, 0x30, 0x1a, 0x1f},
    [SPECIES_MEOWTH          - 1] = {0x1c, 0x16, 0x30, 0x1c, 0x23},
    [SPECIES_PERSIAN         - 1] = {0x13, 0x13, 0x30, 0x13, 0x21},
    [SPECIES_PSYDUCK         - 1] = {0x1c, 0x13, 0x30, 0x17, 0x29},
    [SPECIES_GOLDUCK         - 1] = {0x17, 0x12, 0x28, 0x17, 0x22},
    [SPECIES_MANKEY          - 1] = {0x1c, 0x15, 0x30, 0x1b, 0x23},
    [SPECIES_PRIMEAPE        - 1] = {0x1e, 0x16, 0x30, 0x1a, 0x27},
    [SPECIES_GROWLITHE       - 1] = {0x14, 0x17, 0x30, 0x14, 0x24},
    [SPECIES_ARCANINE        - 1] = {0x0f, 0x09, 0x28, 0x0c, 0x19},
    [SPECIES_POLIWAG         - 1] = {0x15, 0x14, 0x30, 0x13, 0x1f},
    [SPECIES_POLIWHIRL       - 1] = {0x1f, 0x10, 0x30, 0x1e, 0x17},
    [SPECIES_POLIWRATH       - 1] = {0x22, 0x0c, 0x30, 0x21, 0x18},
    [SPECIES_ABRA            - 1] = {0x1f, 0x13, 0x30, 0x1f, 0x21},
    [SPECIES_KADABRA         - 1] = {0x1b, 0x12, 0x30, 0x1c, 0x25},
    [SPECIES_ALAKAZAM        - 1] = {0x1c, 0x11, 0x28, 0x1d, 0x26},
    [SPECIES_MACHOP          - 1] = {0x1b, 0x11, 0x30, 0x1b, 0x1e},
    [SPECIES_MACHOKE         - 1] = {0x1d, 0x11, 0x30, 0x1d, 0x1f},
    [SPECIES_MACHAMP         - 1] = {0x1a, 0x0d, 0x28, 0x1c, 0x1b},
    [SPECIES_BELLSPROUT      - 1] = {0x15, 0x0e, 0x30, 0x0e, 0x1c},
    [SPECIES_WEEPINBELL      - 1] = {0x1a, 0x1c, 0x10, 0x15, 0x32},
    [SPECIES_VICTREEBEL      - 1] = {0x1e, 0x0f, 0x30, 0x1c, 0x1b},
    [SPECIES_TENTACOOL       - 1] = {0x1a, 0x15, 0x30, 0x1e, 0x26},
    [SPECIES_TENTACRUEL      - 1] = {0x1b, 0x10, 0x28, 0x1c, 0x26},
    [SPECIES_GEODUDE         - 1] = {0x1c, 0x20, 0x28, 0x1d, 0x2d},
    [SPECIES_GRAVELER        - 1] = {0x1d, 0x19, 0x28, 0x1e, 0x26},
    [SPECIES_GOLEM           - 1] = {0x19, 0x1b, 0x28, 0x1c, 0x27},
    [SPECIES_PONYTA          - 1] = {0x0f, 0x11, 0x30, 0x0e, 0x23},
    [SPECIES_RAPIDASH        - 1] = {0x2f, 0x0f, 0x28, 0x2b, 0x1e},
    [SPECIES_SLOWPOKE        - 1] = {0x14, 0x1f, 0x28, 0x15, 0x2f},
    [SPECIES_SLOWBRO         - 1] = {0x0e, 0x0d, 0x28, 0x0c, 0x1f},
    [SPECIES_MAGNEMITE       - 1] = {0x20, 0x1b, 0x10, 0x1f, 0x29},
    [SPECIES_MAGNETON        - 1] = {0x1f, 0x14, 0x08, 0x1f, 0x22},
    [SPECIES_FARFETCHD       - 1] = {0x16, 0x14, 0x30, 0x16, 0x29},
    [SPECIES_DODUO           - 1] = {0x11, 0x16, 0x30, 0x0d, 0x2c},
    [SPECIES_DODRIO          - 1] = {0x1d, 0x04, 0x28, 0x0e, 0x07},
    [SPECIES_SEEL            - 1] = {0x1b, 0x24, 0x20, 0x17, 0x35},
    [SPECIES_DEWGONG         - 1] = {0x12, 0x0a, 0x30, 0x0e, 0x1a},
    [SPECIES_GRIMER          - 1] = {0x1f, 0x10, 0x30, 0x1d, 0x25},
    [SPECIES_MUK             - 1] = {0x17, 0x0f, 0x30, 0x19, 0x22},
    [SPECIES_SHELLDER        - 1] = {0x1a, 0x18, 0x30, 0x17, 0x29},
    [SPECIES_CLOYSTER        - 1] = {0x19, 0x1e, 0x28, 0x1d, 0x2c},
    [SPECIES_GASTLY          - 1] = {0x1a, 0x1f, 0x08, 0x19, 0x30},
    [SPECIES_HAUNTER         - 1] = {0x20, 0x14, 0x08, 0x1d, 0x26},
    [SPECIES_GENGAR          - 1] = {0x1a, 0x16, 0x30, 0x19, 0x28},
    [SPECIES_ONIX            - 1] = {0x1f, 0x24, 0x20, 0x28, 0x36},
    [SPECIES_DROWZEE         - 1] = {0x1a, 0x0e, 0x30, 0x1a, 0x1e},
    [SPECIES_HYPNO           - 1] = {0x1c, 0x12, 0x30, 0x19, 0x24},
    [SPECIES_KRABBY          - 1] = {0x1d, 0x1d, 0x30, 0x1a, 0x2b},
    [SPECIES_KINGLER         - 1] = {0x1e, 0x1f, 0x28, 0x1c, 0x30},
    [SPECIES_VOLTORB         - 1] = {0x1c, 0x1b, 0x30, 0x1b, 0x2b},
    [SPECIES_ELECTRODE       - 1] = {0x1c, 0x18, 0x30, 0x1d, 0x2e},
    [SPECIES_EXEGGCUTE       - 1] = {0x21, 0x17, 0x30, 0x20, 0x24},
    [SPECIES_EXEGGUTOR       - 1] = {0x1c, 0x17, 0x28, 0x1b, 0x25},
    [SPECIES_CUBONE          - 1] = {0x1b, 0x12, 0x30, 0x1b, 0x22},
    [SPECIES_MAROWAK         - 1] = {0x14, 0x0f, 0x30, 0x11, 0x23},
    [SPECIES_HITMONLEE       - 1] = {0x2e, 0x11, 0x30, 0x28, 0x22},
    [SPECIES_HITMONCHAN      - 1] = {0x22, 0x0e, 0x30, 0x1e, 0x1d},
    [SPECIES_LICKITUNG       - 1] = {0x14, 0x11, 0x30, 0x14, 0x24},
    [SPECIES_KOFFING         - 1] = {0x1e, 0x14, 0x10, 0x1d, 0x20},
    [SPECIES_WEEZING         - 1] = {0x11, 0x0d, 0x10, 0x0f, 0x1c},
    [SPECIES_RHYHORN         - 1] = {0x0e, 0x23, 0x20, 0x14, 0x33},
    [SPECIES_RHYDON          - 1] = {0x0d, 0x12, 0x28, 0x17, 0x22},
    [SPECIES_CHANSEY         - 1] = {0x1a, 0x0d, 0x30, 0x1a, 0x1a},
    [SPECIES_TANGELA         - 1] = {0x1c, 0x18, 0x30, 0x1d, 0x29},
    [SPECIES_KANGASKHAN      - 1] = {0x1b, 0x06, 0x28, 0x1c, 0x14},
    [SPECIES_HORSEA          - 1] = {0x1c, 0x14, 0x30, 0x16, 0x23},
    [SPECIES_SEADRA          - 1] = {0x10, 0x17, 0x30, 0x09, 0x2a},
    [SPECIES_GOLDEEN         - 1] = {0x16, 0x23, 0x20, 0x15, 0x34},
    [SPECIES_SEAKING         - 1] = {0x16, 0x0f, 0x30, 0x10, 0x18},
    [SPECIES_STARYU          - 1] = {0x1c, 0x18, 0x30, 0x1a, 0x26},
    [SPECIES_STARMIE         - 1] = {0x1d, 0x13, 0x30, 0x1b, 0x20},
    [SPECIES_MR_MIME         - 1] = {0x24, 0x11, 0x30, 0x25, 0x1e},
    [SPECIES_SCYTHER         - 1] = {0x10, 0x19, 0x28, 0x12, 0x27},
    [SPECIES_JYNX            - 1] = {0x1b, 0x0f, 0x30, 0x19, 0x1c},
    [SPECIES_ELECTABUZZ      - 1] = {0x14, 0x11, 0x30, 0x17, 0x20},
    [SPECIES_MAGMAR          - 1] = {0x10, 0x0e, 0x30, 0x0b, 0x19},
    [SPECIES_PINSIR          - 1] = {0x1d, 0x1c, 0x28, 0x20, 0x2b},
    [SPECIES_TAUROS          - 1] = {0x11, 0x1d, 0x28, 0x13, 0x2e},
    [SPECIES_MAGIKARP        - 1] = {0x16, 0x15, 0x30, 0x13, 0x23},
    [SPECIES_GYARADOS        - 1] = {0x0b, 0x18, 0x28, 0x16, 0x29},
    [SPECIES_LAPRAS          - 1] = {0x16, 0x08, 0x28, 0x15, 0x16},
    [SPECIES_DITTO           - 1] = {0x24, 0x12, 0x30, 0x1f, 0x20},
    [SPECIES_EEVEE           - 1] = {0x16, 0x16, 0x30, 0x16, 0x23},
    [SPECIES_VAPOREON        - 1] = {0x18, 0x23, 0x28, 0x18, 0x30},
    [SPECIES_JOLTEON         - 1] = {0x14, 0x1b, 0x30, 0x16, 0x28},
    [SPECIES_FLAREON         - 1] = {0x16, 0x17, 0x30, 0x14, 0x25},
    [SPECIES_PORYGON         - 1] = {0x18, 0x12, 0x30, 0x10, 0x23},
    [SPECIES_OMANYTE         - 1] = {0x1d, 0x1d, 0x30, 0x1c, 0x2b},
    [SPECIES_OMASTAR         - 1] = {0x16, 0x1a, 0x30, 0x19, 0x2d},
    [SPECIES_KABUTO          - 1] = {0x1b, 0x16, 0x30, 0x1a, 0x27},
    [SPECIES_KABUTOPS        - 1] = {0x15, 0x11, 0x30, 0x15, 0x1f},
    [SPECIES_AERODACTYL      - 1] = {0x13, 0x1d, 0x00, 0x14, 0x30},
    [SPECIES_SNORLAX         - 1] = {0x35, 0x09, 0x30, 0x2e, 0x14},
    [SPECIES_ARTICUNO        - 1] = {0x18, 0x0b, 0x18, 0x17, 0x17},
    [SPECIES_ZAPDOS          - 1] = {0x0e, 0x0e, 0x10, 0x04, 0x28},
    [SPECIES_MOLTRES         - 1] = {0x0b, 0x2a, 0x08, 0x0b, 0x38},
    [SPECIES_DRATINI         - 1] = {0x0d, 0x12, 0x30, 0x0b, 0x21},
    [SPECIES_DRAGONAIR       - 1] = {0x18, 0x0e, 0x30, 0x13, 0x1c},
    [SPECIES_DRAGONITE       - 1] = {0x1b, 0x07, 0x18, 0x19, 0x17},
    [SPECIES_MEWTWO          - 1] = {0x2b, 0x09, 0x28, 0x2c, 0x18},
    [SPECIES_MEW             - 1] = {0x19, 0x14, 0x10, 0x1f, 0x21},
};

void AddTextPrinterParameterized3(u8 windowId, u8 fontId, u8 x, u8 y, const u8 * color, s8 speed, const u8 * str)
{
    struct TextPrinterTemplate printer;

    printer.currentChar = str;
    printer.windowId = windowId;
    printer.fontId = fontId;
    printer.x = x;
    printer.y = y;
    printer.currentX = printer.x;
    printer.currentY = printer.y;
    printer.letterSpacing = GetFontAttribute(fontId, 2);
    printer.lineSpacing = GetFontAttribute(fontId, 3);
    printer.unk = 0;
    printer.fgColor = color[1];
    printer.bgColor = color[0];
    printer.shadowColor = color[2];
    AddTextPrinter(&printer, speed, NULL);
}

void AddTextPrinterParameterized4(u8 windowId, u8 fontId, u8 x, u8 y, u8 letterSpacing, u8 lineSpacing, const u8 *color, s8 speed, const u8 *str)
{
    struct TextPrinterTemplate printer;

    printer.currentChar = str;
    printer.windowId = windowId;
    printer.fontId = fontId;
    printer.x = x;
    printer.y = y;
    printer.currentX = printer.x;
    printer.currentY = printer.y;
    printer.letterSpacing = letterSpacing;
    printer.lineSpacing = lineSpacing;
    printer.unk = 0;
    printer.fgColor = color[1];
    printer.bgColor = color[0];
    printer.shadowColor = color[2];
    AddTextPrinter(&printer, speed, NULL);
}

void AddTextPrinterParameterized5(u8 windowId, u8 fontId, const u8 *str, u8 x, u8 y, u8 speed, void (*callback)(struct TextPrinterTemplate *, u16), u8 letterSpacing, u8 lineSpacing)
{
    struct TextPrinterTemplate printer;

    printer.currentChar = str;
    printer.windowId = windowId;
    printer.fontId = fontId;
    printer.x = x;
    printer.y = y;
    printer.currentX = x;
    printer.currentY = y;
    printer.letterSpacing = letterSpacing;
    printer.lineSpacing = lineSpacing;
    printer.unk = 0;
    printer.fgColor = GetFontAttribute(fontId, 5);
    printer.bgColor = GetFontAttribute(fontId, 6);
    printer.shadowColor = GetFontAttribute(fontId, 7);
    AddTextPrinter(&printer, speed, callback);
}

void Menu_PrintFormatIntlPlayerName(u8 windowId, const u8 * src, u16 x, u16 y)
{
    s32 i;

    for (i = 0; gSaveBlock2Ptr->playerName[i] != EOS; i++)
        ;

    StringExpandPlaceholders(gStringVar4, src);
    if (i != 5)
    {
        AddTextPrinterParameterized(windowId, 2, gStringVar4, x, y, 0xFF, NULL);
    }
    else
    {
        AddTextPrinterParameterized5(windowId, 2, gStringVar4, x, y, 0xFF, NULL, 0, 0);
    }
}

static void sub_812E768(const struct Bitmap *src, struct Bitmap *dst, u16 srcX, u16 srcY, u16 dstX, u16 dstY, u16 width, u16 height)
{
    s32 loopSrcY, loopDstY, loopSrcX, loopDstX, xEnd, yEnd, multiplierSrcY, multiplierDstY;
    u16 toOrr;
    u8 *pixelsSrc;
    u8 *pixelsDst;

    if (dst->width - dstX < width)
        xEnd = dst->width - dstX + srcX;
    else
        xEnd = width + srcX;

    if (dst->height - dstY < height)
        yEnd = srcY + dst->height - dstY;
    else
        yEnd = srcY + height;
    multiplierSrcY = (src->width + (src->width & 7)) >> 3;
    multiplierDstY = (dst->width + (dst->width & 7)) >> 3;
    for (loopSrcY = srcY, loopDstY = dstY; loopSrcY < yEnd; loopSrcY++, loopDstY++)
    {
        for (loopSrcX = srcX, loopDstX = dstX; loopSrcX < xEnd; loopSrcX++, loopDstX++)
        {
            pixelsSrc = (u8 *)(src->pixels + ((loopSrcX >> 1) & 3) + ((loopSrcX >> 3) << 5) + (((loopSrcY >> 3) * multiplierSrcY) << 5) + ((u32)(loopSrcY << 0x1d) >> 0x1B));
            pixelsDst = (u8 *)(dst->pixels + ((loopDstX >> 1) & 3) + ((loopDstX >> 3) << 5) + ((( loopDstY >> 3) * multiplierDstY) << 5) + ((u32)( loopDstY << 0x1d) >> 0x1B));

            if ((u32)pixelsDst & 0x1)
            {
                pixelsDst--;
                if (loopDstX & 0x1)
                {
                    toOrr = *(vu16 *)pixelsDst & 0x0fff;
                    if (loopSrcX & 0x1)
                        toOrr |= ((*pixelsSrc & 0xf0) << 8);
                    else
                        toOrr |= ((*pixelsSrc & 0x0f) << 12);
                }
                else
                {
                    toOrr = *(vu16 *)pixelsDst & 0xf0ff;
                    if (loopSrcX & 0x1)
                        toOrr |= ((*pixelsSrc & 0xf0) << 4);
                    else
                        toOrr |= ((*pixelsSrc & 0x0f) << 8);
                }
            }
            else
            {
                if (loopDstX & 1)
                {
                    toOrr = *(vu16 *)pixelsDst & 0xff0f;
                    if (loopSrcX & 1)
                        toOrr |= ((*pixelsSrc & 0xf0) << 0);
                    else
                        toOrr |= ((*pixelsSrc & 0x0f) << 4);
                }
                else
                {
                    toOrr = *(vu16 *)pixelsDst & 0xfff0;
                    if (loopSrcX & 1)
                        toOrr |= ((*pixelsSrc & 0xf0) >> 4);
                    else
                        toOrr |= ((*pixelsSrc & 0x0f) >> 0);
                }
            }
            *(vu16 *)pixelsDst = toOrr;
        }
    }
}

#define tEvA data[0]
#define tEvB data[1]
#define tEvAEnd data[2]
#define tEvBEnd data[3]
#define tEvADelta data[4]
#define tEvBDelta data[5]
#define tEvWhich data[6]
#define tEvStepCount data[8]

void StartBlendTask(u8 eva_start, u8 evb_start, u8 eva_end, u8 evb_end, u8 ev_step, u8 priority)
{
    u8 taskId = CreateTask(Task_SmoothBlendLayers, priority);
    gTasks[taskId].tEvA = eva_start << 8;
    gTasks[taskId].tEvB = evb_start << 8;
    gTasks[taskId].tEvAEnd = eva_end;
    gTasks[taskId].tEvBEnd = evb_end;
    gTasks[taskId].tEvADelta = (eva_end - eva_start) * 256 / ev_step;
    gTasks[taskId].tEvBDelta = (evb_end - evb_start) * 256 / ev_step;
    gTasks[taskId].tEvStepCount = ev_step;
    SetGpuReg(REG_OFFSET_BLDALPHA, (evb_start << 8) | eva_start);
}

bool8 IsBlendTaskActive(void)
{
    return FuncIsActiveTask(Task_SmoothBlendLayers);
}

static void Task_SmoothBlendLayers(u8 taskId)
{
    s16 * data = gTasks[taskId].data;

    if (tEvStepCount != 0)
    {
        if (tEvWhich == 0)
        {
            tEvA += tEvADelta;
            tEvWhich = 1;
        }
        else
        {
            if (--tEvStepCount != 0)
            {
                tEvB += tEvBDelta;
            }
            else
            {
                tEvA = tEvAEnd << 8;
                tEvB = tEvBEnd << 8;
            }
            tEvWhich = 0;
        }
        SetGpuReg(REG_OFFSET_BLDALPHA, (tEvB & ~0xFF) | ((u16)tEvA >> 8));
        if (tEvStepCount == 0)
            DestroyTask(taskId);
    }
}

u8 Menu2_GetMonSpriteAnchorCoord(u16 species, u32 personality, u8 a2)
{
    if (species != SPECIES_NONE && a2 < 5)
    {
        species--;
        if (sMonSpriteAnchorCoords[species][a2] != 0xFF)
            return sMonSpriteAnchorCoords[species][a2];
    }
    return 32;
}

s8 Menu2_GetMonSpriteAnchorCoordMinusx20(u16 species, u32 personality, u8 a2)
{
    return Menu2_GetMonSpriteAnchorCoord(species, personality, a2) - 32;
}
