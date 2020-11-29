#if !defined(NOMEDIA_ANIM_H)
#define NOMEDIA_ANIM_H

#if !defined(NOMEDIA_OLDSTYLE)
/* These coordinates define the Main "Logo" region of the picture */

#define BOOTLOGO_X		0
#define BOOTLOGO_Y		0
#define BOOTLOGO_WIDTH	479
#define BOOTLOGO_HEIGHT	144

/* These coordinates define the AROS logo region of the picture */

#define AROSLOGO_X		0
#define AROSLOGO_Y		145
#define AROSLOGO_WIDTH	266
#define AROSLOGO_HEIGHT	203

/* These coordinates define the media region(s) of the picture */

#define MEDIALOGO_X		267
#define MEDIALOGO_Y		146
#define MEDIALOGO_WIDTH	212
#define MEDIALOGO_HEIGHT	57

/* These coordinates define the flashing region of the picture */

#define FLASH_X		        MEDIALOGO_X
#define FLASH_Y		        MEDIALOGO_Y
#define FLASH_WIDTH	        MEDIALOGO_WIDTH
#define FLASH_HEIGHT	        MEDIALOGO_HEIGHT

#else

/* Coordinates defining the Old flashing region of the picture */

#define FLASH_X		        384
#define FLASH_Y		        0
#define FLASH_WIDTH	        30
#define FLASH_HEIGHT	        52

#endif
#endif /* !NOMEDIA_ANIM_H */
