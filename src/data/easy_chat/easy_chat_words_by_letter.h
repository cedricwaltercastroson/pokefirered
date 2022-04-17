#include "easy_chat.h"
#include "constants/easy_chat.h"

static const u16 sEasyChatWordsByLetter_Others[] = {
    EC_WORD_EXCL,
    EC_WORD_EXCL_EXCL,
    EC_WORD_DASH,
    EC_WORD_DASH_DASH_DASH,
    EC_WORD_ELLIPSIS,
    EC_WORD_ELLIPSIS_EXCL,
    EC_WORD_ELLIPSIS_ELLIPSIS_ELLIPSIS,
    EC_WORD_1_HIT_KO_EXCL,
    EC_WORD_QUES,
    EC_WORD_QUES_EXCL,
};

static const u16 sEasyChatWordsByLetter_A[] = {
    EC_WORD_A,
    EC_WORD_A_LITTLE,
    EC_WORD_A_LOT,
    EC_WORD_A_TINY_BIT,
    EC_WORD_ABOUT,
    EC_WORD_ABOVE,
};

static const u16 sEasyChatWordsByLetter_B[] = {
    EC_WORD_BABE,
    EC_WORD_BABY,
    EC_WORD_BACK,
    EC_WORD_BAD,
    EC_WORD_BADLY,
    EC_WORD_BAG,
    EC_WORD_BYE_BYE,
};

static const u16 sEasyChatWordsByLetter_C[] = {
    
};

static const u16 sEasyChatWordsByLetter_D[] = {
    EC_WORD_DAILY,
    EC_WORD_DAMP,
    EC_WORD_DANCE,
    EC_WORD_DANGER,
    EC_WORD_DARK,
    EC_WORD_DARN,
    EC_WORD_DATE,
    EC_WORD_DAUGHTER,
    EC_WORD_DAY,
    EC_WORD_DAYS,
    EC_WORD_DAYTIME,
    EC_WORD_DECIDE,
    EC_WORD_DECIDED,
    EC_WORD_DEEP,
    EC_WORD_DEFEATED,
};

static const u16 sEasyChatWordsByLetter_E[] = {
    EC_WORD_EARLIER,
    EC_WORD_EARLY_BIRD,
};

static const u16 sEasyChatWordsByLetter_F[] = {
    EC_WORD_FABULOUS,
};

static const u16 sEasyChatWordsByLetter_G[] = {
    EC_WORD_GAME,
};

static const u16 sEasyChatWordsByLetter_H[] = {
    EC_WORD_HABIT,
    EC_WORD_HAH,
    EC_WORD_HAHAHA,
};

static const u16 sEasyChatWordsByLetter_I[] = {
    EC_WORD_I,
    EC_WORD_I_AM,
    EC_WORD_I_CHOOSE_YOU,
    EC_WORD_I_WAS,
    EC_WORD_I_VE,
    EC_WORD_I_VE_ARRIVED,
    EC_WORD_ICE,
};

static const u16 sEasyChatWordsByLetter_J[] = {

};

static const u16 sEasyChatWordsByLetter_K[] = {

};

static const u16 sEasyChatWordsByLetter_L[] = {
    EC_WORD_LACKING,
    EC_WORD_LACKS,
    EC_WORD_LADY,
};

static const u16 sEasyChatWordsByLetter_M[] = {
    EC_WORD_MUSIC,
    EC_WORD_MUST_BE,
    EC_WORD_MY,
    EC_WORD_MYSELF,
    EC_WORD_MYSTERY,
};

static const u16 sEasyChatWordsByLetter_N[] = {
    EC_WORD_NAME,
    EC_WORD_NAP,
};

static const u16 sEasyChatWordsByLetter_O[] = {
    EC_WORD_OBLIVIOUS,
    EC_WORD_OVERWHELMING,
    EC_WORD_OWN,
    EC_WORD_OWN_TEMPO,
};

static const u16 sEasyChatWordsByLetter_P[] = {
    EC_WORD_PUSHOVER,
};

static const u16 sEasyChatWordsByLetter_Q[] = {

};

static const u16 sEasyChatWordsByLetter_R[] = {
    EC_WORD_RADIO,
    EC_WORD_ROUGH_SKIN,
    EC_WORD_RPG,
    EC_WORD_RUBY,
    EC_WORD_RULE,
    EC_WORD_RUN,
    EC_WORD_RUN_AWAY,
    EC_WORD_RUNS,
};

static const u16 sEasyChatWordsByLetter_S[] = {
    EC_WORD_SYSTEM,
};

static const u16 sEasyChatWordsByLetter_T[] = {

};

static const u16 sEasyChatWordsByLetter_U[] = {
    EC_WORD_UGLY,
    EC_WORD_UH_HUH,
    EC_WORD_UH_OH,
    EC_WORD_UM,
    EC_WORD_USE,
    EC_WORD_USELESS,
    EC_WORD_USES,
    EC_WORD_USING,
};

static const u16 sEasyChatWordsByLetter_V[] = {
    EC_WORD_VACATION,
};

static const u16 sEasyChatWordsByLetter_W[] = {
    EC_WORD_WAAAH,
    EC_WORD_WAHAHAHA,
};

static const u16 sEasyChatWordsByLetter_X[] = {
    
};

static const u16 sEasyChatWordsByLetter_Y[] = {
    EC_WORD_YAHOO,
    EC_WORD_YAY,
    EC_WORD_YEAH,
    EC_WORD_YEAH_YEAH,
    EC_WORD_YEEHAW_EXCL,
    EC_WORD_YES,
    EC_WORD_YES_SIR_EXCL,
    EC_WORD_YESTERDAY,
    EC_WORD_YET,
    EC_WORD_YO,
    EC_WORD_YOU,
    EC_WORD_YOU_RE,
    EC_WORD_YOU_VE,
    EC_WORD_YOUNG,
    EC_WORD_YOUR,
    EC_WORD_YOURS,
    EC_WORD_YUP,
};

static const u16 sEasyChatWordsByLetter_Z[] = {

};

static const struct EasyChatWordsByLetter sEasyChatWordsByLetterPointers[] = {
    {
        .words = sEasyChatWordsByLetter_Others,
        .numWords = 10,
    },
    {
        .words = sEasyChatWordsByLetter_A,
        .numWords = 106,
    },
    {
        .words = sEasyChatWordsByLetter_B,
        .numWords = 86,
    },
    {
        .words = sEasyChatWordsByLetter_C,
        .numWords = 119,
    },
    {
        .words = sEasyChatWordsByLetter_D,
        .numWords = 94,
    },
    {
        .words = sEasyChatWordsByLetter_E,
        .numWords = 58,
    },
    {
        .words = sEasyChatWordsByLetter_F,
        .numWords = 79,
    },
    {
        .words = sEasyChatWordsByLetter_G,
        .numWords = 98,
    },
    {
        .words = sEasyChatWordsByLetter_H,
        .numWords = 101,
    },
    {
        .words = sEasyChatWordsByLetter_I,
        .numWords = 48,
    },
    {
        .words = sEasyChatWordsByLetter_J,
        .numWords = 13,
    },
    {
        .words = sEasyChatWordsByLetter_K,
        .numWords = 33,
    },
    {
        .words = sEasyChatWordsByLetter_L,
        .numWords = 82,
    },
    {
        .words = sEasyChatWordsByLetter_M,
        .numWords = 142,
    },
    {
        .words = sEasyChatWordsByLetter_N,
        .numWords = 54,
    },
    {
        .words = sEasyChatWordsByLetter_O,
        .numWords = 44,
    },
    {
        .words = sEasyChatWordsByLetter_P,
        .numWords = 102,
    },
    {
        .words = sEasyChatWordsByLetter_Q,
        .numWords = 6,
    },
    {
        .words = sEasyChatWordsByLetter_R,
        .numWords = 80,
    },
    {
        .words = sEasyChatWordsByLetter_S,
        .numWords = 262,
    },
    {
        .words = sEasyChatWordsByLetter_T,
        .numWords = 133,
    },
    {
        .words = sEasyChatWordsByLetter_U,
        .numWords = 24,
    },
    {
        .words = sEasyChatWordsByLetter_V,
        .numWords = 33,
    },
    {
        .words = sEasyChatWordsByLetter_W,
        .numWords = 111,
    },
    {
        .words = sEasyChatWordsByLetter_X,
        .numWords = 4,
    },
    {
        .words = sEasyChatWordsByLetter_Y,
        .numWords = 19,
    },
    {
        .words = sEasyChatWordsByLetter_Z,
        .numWords = 8,
    },
};
