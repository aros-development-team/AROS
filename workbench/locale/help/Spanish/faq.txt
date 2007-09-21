=====================================
Respuestas a las Preguntas Frecuentes
=====================================

:Authors:   Aaron Digulla, Adam Chodorowski, Sergey Mineychev, AROS-Exec.org
:Copyright: Copyright Š 1995-2007, The AROS Development Team
:Version:   $Revision: 26368 $
:Date:      $Date: 2007-07-30 06:31:59 +1000 (Mon, 30 Jul 2007) $
:Status:    Done.

.. Contents::

Preguntas comunes
=================

¿Puedo hacer una pregunta?
--------------------------

Por supuesto. Por favor ve al `foro de AROS-Exec 
<http://aros-exec.org/modules/newbb/viewtopic.php?topic_id=1636&start=0>`__ 
y lee los hilos y pregunta todo lo que quieras. Este FAQ se actualizará con
las respuestas de los usuarios, aunque el foro permanece más actual. 

¿Qué es AROS?
-------------

Por favor lee la introducción_.

.. _introducción: ../../introduction/index


¿Cuál es el estado legal de AROS?
---------------------------------

Las leyes europeas dicen que es legal usar técnicas de ingeniería inversa
para conseguir interoperatibilidad. También dice que es ilegal distribuir el 
conocimiento obtenido con esas técnicas. Esto básicamente significa que te 
está permitido desensamblar o resource cualquier software para escribir 
algo que sea compatible (por ejemplo, sería legal desensamblar Word para 
escribir un programa que convierte los documentos de Word en texto ASCII).

Por supuesto que hay limitaciones: no te está permitido desensamblar el 
software si la información que tú conseguirías por este proceso puede ser 
obtenida por otros medios. Además no debes contar lo que aprendiste. Un 
libro como "Windows por dentro" es por lo tanto ilegal o al menos legalmente dudoso.

Desde que nosotros evitamos las técnicas de desensamblado y en su lugar 
usamos el conocimiento común disponible (lo que incluye a los manuales de 
programación) que no cae dentro de ningún Acuerdo de No Divulgación, lo de 
arriba no se aplica directamente a AROS. Lo que cuenta aquí es la intención 
de la ley: es legal escribir software que sea compatible con algún otro software. 
Por lo tanto creemos que AROS está protegido por la ley.

Aunque las patentes y los archivos de cabecera son un asunto diferente. Podemos 
usar algoritmos patentados en Europa ya que las leyes europeas no permiten patentar 
los algoritmos. Sin embargo, el código que usa tales algoritmos que están patentados 
en EE.UU. no podría ser importado a EE.UU. Ejemplos de algoritmos patentados en 
AmigaOS incluyen el arrastrado en la pantalla y la manera específica en que funcionan 
los menús. Por otro lado, los archivos de cabecera deben ser compatibles pero tan 
diferentes como sea posible de los originales.

Para evitar cualquier problema nosotros solicitamos un oficial vistobueno de 
Amiga Inc. Ellos son muy positivos respecto del esfuerzo pero están muy preocupados
respecto a las implicaciones legales. Te sugerimos que tomes en cuenta el hecho 
que Amiga Inc. no nos envió ninguna carta "de cesar y desistir" como un signo 
positivo. Desafortunadamente, todavía no hubo ningún acuerdo que sonara legal, 
a pesar de las buenas intenciones de ambas partes.

¿Por qué solamente apuntan a ser compatibles con AmigaOS 3.1?
-------------------------------------------------------------

Hubo discusiones acerca de escribir un avanzado OS con las característica de AmigaOS. 
Se ha dejado esto por una buena razón. Primero, todos están de acuerdo que el 
actual AmigaOS debería ser mejorado, pero nadie sabe cómo hacerlo o se pone de 
acuerdo sobre qué tiene que ser mejorado o qué es importante. Por ejemplo, algunos 
quieren protección de memoria, pero no quieren pagar el precio (una reescritura 
mayor del software disponible y una disminución de la velocidad).

Al final, las discusiones terminaron en guerras de palabras o en la reiteración 
de los mismos viejos argumentos una y otra vez. Así que decidimos empezar con 
algo que sabíamos cómo manejar. Entonces, cuando tuviéramos la experiencia para 
ver qué es posible y qué no, decidiríamos sobre las mejoras.

También queremos ser compatible a nivel binario con el original AmigaOS sobre el Amiga.
La razón para esto es que un nuevo OS sin ningún programa que ejecutar no tiene 
ninguna oportunidad de sobrevivir. Por lo tanto intentamos hacer el paso del OS 
original a uno nuevo tan indoloro como sea posible (pero no al extremo que no 
podamos mejorar AROS después). Como es usual, todo tiene su precio y nosotros 
intentamos decidir cuidadosamente cuál podría ser ese precio y si nosotros y todos 
los demás desearemos pagarlo.

¿Puedes implementar la característica XYZ?
--------------------------------------------------

No, porque:

a) Si fuera realmente importante, ya estaría en el OS original. :-)
b) ¿Por qué no lo haces por tí mismo y nos envías un parche?

La razón para esta actitud es que hay mucha gente alrededor que cree que su 
característica es la más importante y que AROS no tiene futuro si esa 
característica no está integrada. Nuestra posición es que AmigaOS, el OS que 
AROS pretende implementar, puede hacer todo lo que un moderno OS debería hacer. 
Vemos que hay áreas donde AmigaOS podría mejorar, pero si hacemos eso, 
¿quién escribiría el resto del OS? Al final, tendríamos muchas agradables 
mejoras al original OS que romperían a la mayoría del software disponible y no 
valdrían nada, porque el resto del OS estaría faltando.

Por lo tanto, decidimos bloquear todo intento de implementar nuevas características 
mayores en el OS hasta que esté más o menos completo. Ahora estamos bastante cerca 
de esa meta, y ya han habido un par de innovaciones implementadas en AROS que 
no estaban disponibles en AmigaOS.

¿Cuán compatible es AROS con AmigaOS?
---------------------------------------

Muy compatible. Esperamos que AROS ejecutará el software existente sobre el 
Amiga sin problemas. Sobre otro hardware, el software existente debe ser 
recompilado. Ofreceremos un preprocesador que podrás usar en tu código que 
cambiará cualquier código que podría romper AROS y/o te advertirá sobre tal código.

Transferir programas de AmigaOS a AROS es hoy sobre todo un asunto de una simple 
recompilación, con el ocasional tweak aquí y allí. Por supuesto hay programas 
para los que esto no es verdad, aunque sí lo es para los más modernos.

¿Para qué arquitecturas de hardware AROS está disponible?
--------------------------------------------------------------

En la actualidad AROS está disponible en un estado bastante usable en modo 
nativo y en modo alojado (en GNU/Linux, y FreeBSD) para la arquitectura i386 
(p.e. los clones compatibles con la IBM PC AT). Hay puertos en camino en 
variados grados de completitud para SUN SPARC (alojado bajo Solaris) y las 
computadoras de mano compatibles con la Palm (en modo nativo).

¿Habrá un puerto de AROS a PPC?
------------------------------- 

Hay actualmente un esfuerzo en camino para adaptar AROS a PPC, inicialmente
alojado en Linux.

¿Por qué están usando Linux y X11?
----------------------------------

Usamos Linux y X11 para acelerar el desarrollo. Por ejemplo, si implementas una
nueva función para abrir una ventana puedes simplemente escribir esa sóla función
y no tienes que escribir las centenares de las otras funciones en layers.library,
graphics.library, unos montón de controladores de dispositivos y lo demás que esa
función podría necesitar.

La meta para AROS es por supuesto ser independiente de Linux y X11 (aunque todavía
sería capaz de ejecutarse en ellos si la gente realmente quiere hacerlo), y eso
se está convirtiendo lentamente en una realidad con las versiones nativas de AROS.
Todavía necesitamos usar Linux para desarrollar, porque algunas herramientas de
desarrollo no han sido transferidas a AROS todavía.


¿Cómo pretendes hacer portátil a AROS?
--------------------------------------

Una de las mayores nuevas características en AROS comparadas con AmigaOS es el
sistema HIDD (Controladores de Dispositivo Independientes del Hardware), que
nos permitirá transferir AROS fácilmente a diferente hardware. Básicamente, las
bibliotecas centrales del OS no afectan el hardware directamente sino que pasan
a través de los HIDDs, que están codificados usando un sistema orientado a objetos
que hace fácil reemplazar los HIDDs y volver a usar el código.


¿Por qué piensas que AROS lo logrará?
-------------------------------------

Hemos oído todo el día de mucha gente que AROS no lo logrará. La mayoría no sabe
lo que estamos haciendo o creen que la Amiga ya está muerta. A los primeros, luego de que les 
explicamos lo que hacemos, la mayoría está de acuerdo en que es posible. Los últimos
hacen más problema. Bien, ¿está la Amiga muerta? Aquellos que todavía usan sus
Amigas probablemente te dirán que no lo está. ¿Tu A500 o A4000 explotó
cuando Commodore entró en bancarrota? ¿Lo hizo cuando a Amiga Technologies le pasó
lo mismo?

El hecho es que hay bastante poco software nuevo desarrollado para la Amiga
(aunque Aminet todavía resuena bastante bien) y que el hardware también
es desarrollado a una menor velocidad (pero los más asombrosos gadgets
parecen surgir ahora). La comunidad Amiga (que todavía vive) parece estar
sentada y esperando. Y si alguien presentara un producto que fuera un poco como 
la Amiga lo fue en 1984, entonces la máquina estará en auge de nuevo. Y quién
sabe, quizás obtengas un CD con la máquina etiquetado "AROS". :-)


¿Qué hago si AROS no quiere compilar?
-------------------------------------


Por favor pon un mensaje con detalles (por ejemplo, los mensajes de error
que obtuviste) en el foro de Ayuda en `AROS-Exec`__ o conviérte en un
desarrollador y suscríbete a la lista AROS Developer y pónlo allí, y
alguien intentará ayudarte.

__ http://aros-exec.org/


¿AROS tendrá memoria protegida, SVM, RT, ...?
---------------------------------------------


Varios centenares de expertos Amiga (eso es lo que ellos piensan de sí mismos
al menos) intentaron por tres años encontrar una manera de implementar memoria
protegida (MP) para el AmigaOS. Fallaron. Deberías tomar como un hecho que un
normal AmigaOS nunca tendrá MP como Unix o Windows NT.

Pero no está todo perdido. Hay planes para integrar una variante de MP en AROS
que permitirá la protección de al menos los nuevos programas que lo conozcan.
Algunos esfuerzos en esta área se ven realmente prometedores. También, ¿es 
realmente un problema si tu máquina se cuelga? Déjame explicartelo, antes de
que tú me claves a un árbol. :-) El problema no es que la máquina se cuelgue,
sino que:

1. No tienes una buena idea de por qué se colgó. Básicamente, terminas
   empujando un poste de 100 pies en un pantano con niebla densa.
2. Perdiste tu trabajo. Reiniciar la máquina no es realmente un problema.

Lo que podríamos intentar construir es un sistema que al menos te alerte si algo
dudosos está pasando y que diga con mucho detalle qué sucederá cuando la 
máquina se cuelgue y que permitirá guardar tu trabajo y *recién entonces* se cuelgue.
Habrá también medios para revisar lo que se guardó así puedas estar seguro que no
sigues con datos corruptos.

La misma cosa va para la SVM (la memoria virtual intercambiable), el RT (el rastreo
de los recursos) y el SMP (el multiprocesamiento simétrico). Estamos planeando
cómo implementarlas, asegurándonos que agregar estas características será indoloro.
Sin embargo, ahora no tienen la prioridad más alta. Aunque se ha agregado un muy
básico RT.


¿Puedo convertirme en un probador beta?
--------------------------------------------

Seguro, no hay problema. De hecho, queremos tantos probadores beta como sea
posible, así que ¡todos son bienvenidos! Aunque no mantenemos una lista de 
los probadores beta, todo lo que tienes que hacer es descargar AROS, probar
lo que quieras y enviarnos un informe.


¿Cuál es la relación entre AROS y UAE?
--------------------------------------

UAE es un emulador de la Amiga, y como tal tiene metas algo diferentes que AROS.
UAE quiere ser compatible con los binarios incluso para los juegos y el código
que afecta al hardware, mientras que AROS quiere tener aplicaciones nativas. Por
lo tanto AROS es mucho más rápido que UAE, pero en UAE puedes ejecutar más 
software.

Tenemos cierto contacto con el autor de UAE y hay una buena oportunidad de que 
el código de UAE aparecerá en AROS y viceversa. Por ejemplo, los desarrolladores
de UAE están interesados en el código fuente del OS porque UAE podría ejecutar
algunas aplicaciones mucho más rápido si alguna o todas las funciones del OS
pueden ser reemplazados con código nativo. Por otra parte, AROS se podría 
beneficiar con tener una emulación del Amiga integrada.

Puesto que la mayoría de los programas no estarán disponibles en AROS desde el 
inicio, Fabio Alemagna ha transferido UAE para AROS para que puedas ejecutar los viejos
programas al menos en una caja de emulación.


¿Cuál es la relación entre AROS y Haage & Partner?
--------------------------------------------------

Haage & Partner usó partes de AROS en AmigaOS 3.5 y 3.9, por ejemplo los gadgets
rueda-de-colores y deslizador-de-gradiente y en el comando SetENV. Esto significa
que de una manera, AROS se ha vuelto parte del oficial AmigaOS. Esto no implica que
hay alguna relación formal entre AROS y Haage & Partner. AROS es un proyecto de 
fuente abierta, y cualquiera puede usar nuestro código en sus propios proyectos
con la estipulación de que cumplan la licencia.


¿Cuál es la relación entre AROS y MorphOS?
------------------------------------------

The relationship between AROS and MorphOS is basically the same as between AROS
and Haage & Partner. MorphOS uses parts of AROS to speed up their development
effort; under the terms of our license. As with Haage & Partner, this is good
for both the teams, since the MorphOS team gets a boost to their development
from AROS and AROS gets good improvements to our source code from the MorphOS
team. There is no formal relation between AROS and MorphOS; this is simply how
open source development works.
La relación entre AROS y MophOS es básicamente la misma que entre AROS y Haage
& Partner. MorhpOS usa partes de AROS para acelerar su esfuerzo de desarrollo;
bajo los términos de nuestra licencia. Como con Haage & Partner, esto es bueno
para ambos equipos, dado que el equipo de MorphOS obtiene de AROS estímulo
para su desarrollo y AROS consigue buenas mejoras para nuestro código fuente del
equipo de MorphOS. No hay una relación formal entre AROS y MorphOs; 
simplemente así es como funciona el desarrollo de fuente abierta.


¿Cuáles lenguajes de programación están disponibles?
----------------------------------------------------

La mayoría del desarrollo de AROS se hace usando ANSI C por compilación
cruzada bajo un OS diferente, p. e. Linux o FreeBSD. Fabio Alemagna ha
completado un puerto inicial de GCC para i386 nativo. Sin embargo, no 
está actualmente en la ISO o incorporado en el build system.

Los lenguajes que están disponibles nativamente son Python_, Regina_ y False_:

+ Python es un lenguaje de scripting que se ha vuelto bastante popular, debido
  a su buen diseño y características (programación orientada a objetos, sistema
  de módulos, muchos módulos útiles incluidos, una sintaxix limpia,...). Se ha 
  iniciado un proyecto separado para el puerto para AROS que se puede encontrar en
  http://pyaros.sourceforge.net/.
  
+ Regina es un interpretador portátil compatible con REXX. La meta para el puerto
  para AROS es ser compatible con el interpretador ARexx del clásico AmigaOS.
  
+ False se puede clasificar como un lenguaje exótico, así que probablemente no
  será usado para el desarrollo serio, aunque puede ser bastante divertido. :-)

.. _Python: http://www.python.org/
.. _Regina: http://regina-rexx.sourceforge.net/
.. _False:  http://wouter.fov120.com/false/


¿Por qué no hay un emulador m68k en AROS?
-----------------------------------------

Para hacer que los viejos programas del Amiga funcionen en AROS, hemos trasferido
UAE_ a AROS. La versión de UAE en AROS probablemente será un poco más rápida que
las otras versiones de UAE ya que AROS necesita menos recursos que otros sistemas
operativos (lo que significa que UAE tendrá más tiempo de CPU), e intentaremos
parchar la ROM Kickstart en UAE para que llame a las funciones de AROS, lo que
dará otra pequeña mejora. Por supuesto, esto solamente se aplica a los sabores
nativos de AROS y no a los sabores alojados.

Pero, ¿por qué no simplemente implementamos una CPU m68k virtual para ejecutar
el software directamente en AROS? Bien, el problema es que el software m68k espera
que los datos estén en formato big endian mientras que AROS también funciona 
en las CPU little endian. El problema es que las rutinas little endian en el 
núcleo de AROS tendrían que funcionar con los datos big endian en la emulación. 
La conversión automática parece ser imposible (sólo un ejemplo: hay un 
campo en una estructura en el AmigaOS que a veces contiene un ULONG y 
a veces dos WORDS) porque no podemos decir cómo están codificados en la 
RAM un par de bytes.

.. _UAE: http://www.freiburg.linux.de/~uae/


¿Habrá una ROM Kicktstart en AROS?
----------------------------------

Podría ser, si alguien crea un puerto nativo Amiga de AROS y hace todo el otro
trabajo necesario para crear una ROM Kickstart. Actualmente, nadie ha solicitado
el trabajo.


Preguntas de software
=====================

¿Cómo accedo a los discos de imagen de AROS desde UAE?
------------------------------------------------------

La imagen de un disquete se puede montar como un archivo de disco duro y después
ser usado como un disco duro de 1,4 MB dentro de UAE. Después que hayas puesto 
los archivos que quieres en la imagen de disco duro (o lo que sea que quieras
hacer), puedes escribirla a un disquete.

La geometría de un archivo de disco duro es la siguiente::

    Sectors    = 32
    Surfaces   = 1
    Reserved   = 2
    Block Size = 90


¿Cómo accedo a las imágenes de disco de AROS desde los sabores alojados de AROS?
-----------------------------------------------------------------------------------

Copia la imagen de disco al directorio DiskImages en AROS (SYS:DiskImages, por ej.
bin/linux-i386/AROS/DiskImages) y cámbiale el nombre a "Unit0". Después de iniciar 
AROS, puedes montar la imagen de disco con::

    > mount AFD0: 


¿Qué es Zune?
-------------

En el caso que leas acerca de Zune en este sitio, es simplemente una reimplementación
fuente abierta de MUI, que es una poderosa biblioteca de GUI shareware orientada a objetos
y la norma de hecho en AmigaOS. Zune es la biblioteca de GUI preferida para desarrollar
aplicaciones nativas de AROS. Respecto al nombre, no significa nada, pero suena bien.

¿Cómo puedo restaurar mis Prefs a las que tenía predeterminadas?
----------------------------------------------------------------

En AROS, abre el shell CLI, ve al Envarc: y borra los archivos que son relevates 
a la preferencia (pref) que quieres restaurar.

¿En el Wanderer, qué es la Graphical Memory y la Other Memory?
---------------------------------------------------------------

Esta división de la memoria es sobre todo una reliquia del pasado del AmigaOS,
cuando la memoria gráfica era la memoria de aplicación antes que tú agregabas otra
llamada FAST RAM, una memoria adonde terminaban las aplicaciones, mientras que los
gráficos, los sonidos y algunas estructuras del sistema quedaban en la memoria gráfica.

En AROS alojado, no hay tal tipo de memoria Other (la FAST), sino solamente GFX,
mientras que en AROS nativo, GFX puede tener un máximo de 16 MB, aunque no refleja
el estado de la memoria del adaptador gráfico... No tiene relación con la cantidad de 
memoria en tu tarjeta gráfica.

*La respuesta más larga*
La memoria gráfica en i386-native se refiere a los 16 MB inferiores de la memoria del
sistema. Esos 16 MB inferiores es el área donde las tarjetas ISA pueden hacer el 
DMA. La asignación de memoria con MEMF_DMA o MEMF_CHIP se hará de allí, la restante
en la otra (FAST) memoria.

Use el comando C:Avail HUMAN para tener información sobre la memoria.

¿Qué hace en realidad la acción Snapshot <all/window> del Wanderer?
-------------------------------------------------------------------

Este comando recuerda la ubicación del ícono de una (o todas) las ventanas.

¿Cómo puedo cambiar el salvapantallas/fondo?
------------------------------------------------

At the moment the only way to change screensaver is to write your one.
Blanker commodity could be tuned with Exchange, but it is able to do only 
"starfield" with given amount of stars.
Background of Wanderer is set by Pref tool Prefs/Wanderer.
Background of Zune Windows is set by Zune prefs Prefs/Zune. You can also set 
your chosen application preferences by using the Zune <application> command.
En este momento la única manera para cambiar el salvapantallas es escribir
el propio. El commodity Blanker podría ser ajustado con Exchange, pero sólo
es capaz de hacer un "campo de estrellas" con una cantidad dada de estrellas.
El Fondo del Wanderer se establece con la herramienta Pref en Prefs/Wanderer.

Al lanzar el AROS alojado falló
-------------------------------

Probablemente esto se podría arreglar creando un directorio WBStartup en el
directorio AROS. Si eres root y AROS se cuelga en el lanzamiento, haz
"xhost +" antes que "sudo && ./aros -m 20". También debes darle algo de 
memoria con la opción -m como se mostró. No te olvides de la opción
BackingStore en la sección Device de tu archivo xorg.conf.

¿Cuáles son las opciones de línea de comandos para el ejecutable del AROS alojado?
----------------------------------------------------------------------------------

Puedes obtener una lista escribiendo el comando ./aros -h.

¿Cómo puedo hacer que las ventanas se refresquen en el AROS alojado?
--------------------------------------------------------------------

Debes proporcionar la siguiene cadena (¡como está!) a tu /etc/X11/xorg.conf (o XFree.conf)::
    
    Option  "BackingStore"

¿Cuáles son las opciones del núcleo de AROS nativo usadas en la línea de GRUB?
------------------------------------------------------------------------------

Aquí están algunas::

    nofdc - Deshabilita por completo el controlador de la disquetera.
    noclick - Deshabilita la detección del cambio de disquete (y el clicking).
    ATA=32bit - Habilita la E/S de 32 bit en el controlador de disco duro (seguro).
    forcedma - Fuerza el DMA activo en el controlador de disco duro (debería ser
	           seguro pero podría no serlo).
    gfx=<nombre del hidd> - Usa el hidd nombrado como el controlador gfx.
    lib=<nombre> - Carga e inicia la biblioteca/hidd nombrado.

Por favor advierte que son sensibles a las mayúsculas.

¿Cómo puedo transferir los archivos a la máquina virtual con AROS?
------------------------------------------------------------------

Primero y lo más fácil es poner los archivos en la imagen ISO y conectarla a la 
MV (máquina virtual). Hay bastante programas capaces de crear/editar ISOs como 
UltraISO, WinImage, o mkisofs. Segundo, puedes configurar la red en AROS y un 
servidor FTP en tu máquina anfitriona. Entonces puedes usar el cliente FTP de 
AROS y transferir los archivos (busca MarranoFTP). Éste es suficientemente 
difícil para detenerse en este punto. La documentación del usuario contiene un 
capítulo sobre redes, vé por él. También, ahora hay una prometedora utilidad 
(AFS Util), que permite leer (todavía no tiene soporte para escribir) archivos 
desde los discos y disquetes AROS AFFS/OFS.

Errores de compilación
----------------------

P: He compilado AROS con gcc4 pero encontré fallas de segmento con -m > 20 en
AROS alojado y si compilo AROS nativo no empieza (la pantalla está negra).
R: Agrega -fno-strict-aliasing al archivo scripts/aros-gcc.in y prueba de nuevo.

¿Es posible hacer un guión DOS que automáticamente se ejecute cuando se instala un paquete?
-------------------------------------------------------------------------------------------

Este guión debería hacer algunas asignaciones y agrega la cadena a la variable PATH.

1) Crea un subdirectorio S y agrega un archivo con el nombre 'Package-Startup' con los
comandos DOS.

2) Crea una variable en el archivo Envarc:SYS/packages que contenga la ruta al directorio
S de tu paquete.

Ejemplo::
    Distribución del directorio:

    sys:Extras/miappdir
    sys:Extras/miappdir/S
    sys:Extras/miappdir/S/Package-Startup
    
La variable en Envarc:sys/packages podría tener el nombre 'miapp' (el nombre no importa),
el contenido sería entonces 'sys:extras/miappdir'.

El guión Package-Startup sería entonces llamado por la startup-sequence (secuencia de inicio).

Así se ve donde es llamado::

    If EXISTS ENV:SYS/Packages
        List ENV:SYS/Packages NOHEAD FILES TO T:P LFORMAT="If EXISTS $SYS/Packages/%s*NCD $SYS/Packages/%s*NIf EXISTS S/Package-Startup*NExecute S/Package-Startup*NEndif*NEndif*N"
        Execute T:P
        Delete T:P QUIET
        CD SYS:
    EndIf
    
¿Cómo puedo limpiar la ventana del shell? ¿Cómo puedo hacerlo de modo permanente?
---------------------------------------------------------------------------------

En el shell tipea este comando::

    Echo "*E[0;0H*E[J* "
    
Puedes editar tu S:Shell-Startup e insertar este renglón en alguna parte,
así tendrás un nuevo comando "Cls"::

    Alias Cls "Echo *"*E[0;0H*E[J*" "

A propósito, aquí está mi nuevo S:Shell-Startup modificado para iniciar el shell
en blanco y con un prompt modificado::

    Alias Edit SYS:Tools/Editor
    Alias Cls "Echo *"*E[0;0H*E[J*" "
    Echo "*e[>1m*e[32;41m*e[0;0H*e[J"
    Prompt "*n*e[>1m*e[33;41m*e[1m%N/%R - *e[30;41m%S>*e[0m*e[32;41m "
    date

Más acerca de las secuencias de escape de la impresora::

    Esc[0m
    Standard Set

    Esc[1m and Esc[22m
    Negrita

    Esc[3m and Esc[23m
    Cursiva

    Esc[4m and Esc[24m
    Subrayado

    Esc[30m to Esc[39m
    Establecer el color de primer plano

    Esc[40m to Esc[49m
    Establecer el color de fondo

Valores significativos::

    30 grey char -- 40 grey cell -- >0 grey background ---- 0 all attributes off
    31 black char - 41 black cell - >1 black background --- 1 boldface
    32 white char - 42 white cell - >2 white background --- 2 faint
    33 blue char -- 43 blue cell -- >3 blue background ---- 3 italic
    34 grey char -- 44 grey cell -- >4 grey background ---- 4 underscore
    35 black char - 45 black cell - >5 black background --- 7 reverse video
    36 white char - 46 white cell - >6 white background --- 8 invisible
    37 blue char -- 47 blue cell -- >7 blue background

Los códigos puedes ser combinados separándolos con un punto y coma.

¿Cómo lanzo AROS alojado a pantalla completa?
---------------------------------------------

En un shell llama "exporte AROS_X11_FULLSCREEN=1". Inicia AROS y cambia la 
resolución de la pantalla en las preferencias de modo de pantalla (screenmode).
Sal de AROS e inícialo de nuevo.

¿Cómo hago los íconos AROS de dos estados?
------------------------------------------

Los íconos de AROS son en realidad archivos PNG renombrados. Pero si quieres íconos
en dos estados (libre/apretado) usa este comando::

    join img_1.png img_2.png TO img.info
    
¿Cómo monto una imagen ISO en AROS? Y ¿puedo puedo poner al día mi nightly build de esta manera?
------------------------------------------------------------------------------------------------

Consigue la ISO en el sitio web de AROS (por medio de wget o de otra manera).
Copia la ISO en sys:DiskImages (el cajón debe ser creado si no existe).
Renombra la ISO a Unit0 en ese directorio.
Debes añadir esto a tu Devs:Mountlist ::

    ISO:
    FileSystem = cdrom.handler
    Device = fdsk.device
    Unit = 0

Después monta la ISO:
Puedes copiar algo desde ISO: 
Por ejemplo, haz un guión para actualizar tu nightly build así::

    *Copy ISO:boot/aros-pc-i386.gz sys:boot/
    *copy ISO:C sys:C all quiet
    *copy ISO:Classes sys:Classes all quiet
    *copy *copy ISO:Demos sys:Demos all quiet

y así para cada directorio excepto Prefs, Extras:Networking/Stacks, y devs:mountlist.
Prefs tiene que ser mantenido si lo quieres. También puedes poner AROSTCP para
mantener sus configuraciones en un directorio separado.

Si quieres escribir por todas partes, haz::

    copy ISO:C sys:C all quiet newer  
    
¿Cómo desmonto un volumen?
--------------------------

En el CLI lanza estos comandos::
    
    assign DOSVOLUME: dismount
    assign DOSVOLUME: remove

donde DOSVOLUME es DH0:, DF0:, etc.

¿Cómo monto un disquete FAT con el FAT.handler?
-----------------------------------------------

Crea un archivo de montaje (un archivo de texto) con los
tres renglones mágicos::

    device = trackdisk.device
    filesystem = fat.handler
    unit = 0

Llámalo de algún modo, PC0 por ejemplo.  Establece en las propiedades de la 
herramienta predeterminada (default tool) de este archivo a C:mount
(o pon el archivo de montaje en devs:dosdrivers o sys:storage/dosdrivers).
Aprieta dos veces en él.
Inserta un disquete formateado a FAT.
Mira aparecer su ícono en el escritorio del Wanderer.

¿Cómo monto una real partición FAT con el FAT.handler?
------------------------------------------------------

Primero necesitas leer la geometría de la unidad y escribir algunos valores.
Puedes usar el HDToolbox o el fdisk de Linux para eso. El valor BlocksPerTrack
se toma del valor sectores/pista. Advierte que no tiene nada que ver con la 
geometría física del disco - FAT solamente lo usa como un multiplicador.

    sudo fdisk -u -l /dev/hda, 
    
Después necesitarás establecer BlocksPerTracks=63.
Para asegurarte que tienes los números en cilindros busca en la salida
Units=Cylinders. Si tienes la salida de fdisk en sectores (Units=sectors),
establecer BlocksPerTracks=1.

LowCyl y HighCyl son los cilindros de la partición y se ven algo así::

    mark@ubuntu:~$ sudo fdisk -l -u /dev/hda
    ...
    /dev/hda1 * 63 20980889 10490413+ c W95 FAT32 (LBA)

Entonces, LowCyl es 63, y HighCyl es 20980889, blockspertrack=1

Crea un archivo de montaje (un archivo de texto) con estos renglones::

    
    device = ata.device
    filesystem = fat.handler,
    Unit = 0

    BlocksPerTrack = 1
    LowCyl = 63
    HighCyl = 20980889
    Blocksize=512

Llámalo de algún modo, FAT0 por ejemplo.
Pon c:mount en las propiedades de la herramienta predeterminada del archivo
(o pon el archivo de montaje en devs:dosdrivers o en sys:storage/dosdrivers).
Aprieta dos veces.
Mira aparecer el ícono en el escritorio del Wanderer

Nota: Fórmula para contar los bloques
block = ((highcyl - lowcyl) x surfaces + head) x blockspertrack + sec

Preguntas sobre el hardware
===========================

¿Dónde puedo encontrar una Lista de Compatibilidad de Hardware para AROS?
-------------------------------------------------------------------------

Puedes encontrar una en la página `AROS Wiki <http://en.wikibooks.org/wiki/Aros/Platforms/x86_support>`__.
Puede haber otras listas hechas por los usuarios de AROS.

¿Por qué AROS no puede arrancar de mi conjunto de unidades como SLAVE en el canal IDE?
--------------------------------------------------------------------------------------

Bueno, AROS debería arrancar si la unidad es SLAVE pero solamente si hay una 
unidad MASTER también. Eso parece ser una conexión correcta respetando la especificación
IDE, y AROS la sigue.

Mi sistema se cuelga con un cursor rojo en la pantalla o con una pantalla negra
--------------------------------------------------------------------------------

Una razón para esto puede ser el uso de un ratón serial (éstos no están soportados
todavía). Debes usar un ratón ps/2 con AROS en este momento. Otra puede que hayas
escogido un modo de video que no está soportado por tu hardware en el menú de arranque.
Reinicia y prueba uno diferente.
