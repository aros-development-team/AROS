#ifndef _LOGOTYPE_H_
#define _LOGOTYPE_H_

/*
    Copyright © 2003-2020, The AROS Development Team. All rights reserved.
    This file is part of the About program, which is distributed under
    the terms of version 2 of the GNU General Public License.
    
    $Id$
*/

//#define LOGO_ASCII_OLD

#define LOGOTYPE_IMAGE "IMAGES:Logos/AROS.logo"
#if !defined(LOGO_ASCII_OLD)
#define LOGOTYPE_ASCII                                                                 "\n" \
"  ,#%%%############*   ,(#####%%#####%%%%%/  ,#%%%###########(             .(##%%%%%%. \n" \
" @/                %% %(                    @/               .%           #%           \n" \
".@                 #% %/                   .@                ##           ##           \n" \
" %&&%%%########,   #% %/                    %@@%%###########%  /#######%&/             \n"
#else
#define LOGOTYPE_ASCII                                  "\n" \
"        _______                 ___________             \n" \
"   _____\\_     \\_  _________ _\\    __    \\ ________ \n" \
"  /     _       /__\\__      \\      /     _/    ___/_  \n" \
" /      /      /    |/      /     /     _\\_____     \\ \n" \
" \\____________/     /      /_\\_________/     /      / \n" \
"           _/       \\_______/         \\____________/  \n" \
"           \\_________|                                 \n" 
#endif
#endif /* _LOGOTYPE_H_ */
