	.rodata

	.globl bgImg
	.balign 32
bgImg:
	.incbin "../data/background"

	.globl cover_top
	.balign 32
cover_top:
	.incbin "../data/cover_top"

	.globl cover_side
	.balign 32
cover_side:
	.incbin "../data/cover_side"

	.globl cover_front
	.balign 32
cover_front:
	.incbin "../data/cover_front"

	.globl coverImg
	.balign 32
coverImg:
	.incbin "../data/nocover"

	.globl coverImg_full
	.balign 32
coverImg_full:
	.incbin "../data/nocover_full"

	.globl pointer
	.balign 32
pointer:
	.incbin "../data/pointer"

	.globl hourglass
	.balign 32
hourglass:
	.incbin "../data/hourglass"

	.globl gui_font
	.balign 32
gui_font:
	.incbin "../data/gui_font"

	.globl bg_gui
	.balign 32
bg_gui:
	.incbin "../data/bg_gui"

	.globl star_icon
	.balign 32
star_icon:
	.incbin "../data/star"

#;	.globl introImg
#;	.balign 32
#;introImg:
#;	.incbin "../data/intro"

	.globl introImg
	.balign 32
introImg:
	.incbin "../data/intro4"

	.globl introImg3
	.balign 32
introImg3:
	.incbin "../data/intro3"

##; commented out
#;	.globl opening_bnr
#;	.balign 32
#;opening_bnr:
#;	.incbin "../data/opening_bnr"
