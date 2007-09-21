==============================
Guía para los Usuarios de AROS
==============================

:Authors:   Stefan Rieken, Matt Parsons, Adam Chodorowski, Sergey Mineychev
:Copyright: Copyright 1995-2006, The AROS Development Team
:Version:   $Revision: 25137 $
:Date:      $Date: 2007-01-05 08:21:43 +1100 (Fri, 05 Jan 2007) $
:Status:    Unfinished; only converted to reST. Needs heavy updating. In works!

.. Warning::

   ¡Este documento está en curso! Es muy probable que algunas partes
   contengan información incorrecta o simplemente falten.
   Si quieres ayudar a rectificar esto, por favor ponte en contacto
   con nosotros.

.. Contents::

Introducción
============

Esta es la Guía para el Usuario del Sistema Operativo de Investigación AROS. 
Es para *todos* los que tengan interés en AROS ya que intenta proporcionar 
la información sobre AROS en diferentes niveles de adelanto. Intentaré cubrir 
todo en profundidad, pero de un modo que no necesites aprender lo que no 
*quieras* aprender.

¿Quién debería leer esta guía?
------------------------------

Esta guía te ayudará a habituarte a AROS. Está escrita para todos los 
interesados en AROS. Ten presente que estás usando software que está en etapa
BETA y en investigación. En este momento es divertido para jugar y brillante para
programar. Entonces espero que tu interés en AROS se explique por una de estas 
dos razones. Si llegaste aquí pensando que AROS es un OS Multimedia listo para
Internet, etc., bueno, podrías estar en lo correcto, pero ---no está finalizado---,
así que sé paciente, chico. Si creías que AROS era una máquina para hacer jugo o
un Proyecto Gratuito, estás por completo en el lugar equivocado.


¿Cómo deberías leer esta guía?
------------------------------

Esta guía está ordenada de "simple" a "avanzada". Puedes empezar a leer
cualquier capítulo que contenga información que es nueva para tí. Pero
quizás más importante, deberías dejar de leer cualquier capítulo que
contenga información que vaya más allá de tu interés. De esta manera
puedes aprender por tí mismo los temas avanzados empezando desde cero,
o puedes detenerte temprano si crees que sólo quieres usar AROS, y no
programar. La gente con formación en Amiga puede saltar la introducción,
y empezar en "Desarrollando para la plataforma de AROS" si nunca han
programado un Amiga antes, o ir directamente a "Desarrollando desde AROS"
si ya lo hicieron. Así que hay un punto de partida y uno de llegada para
todos.

Es importante comprender que esta guía es sobre AROS, no sobre Amiga.
Entonces si posees un Amiga de hace años, también podrías necesitar leer
"Usando AROS". Esto no es para avergonzarse: notarás que usar AROS es
ligeramente diferente de usar AmigaOS. Esto se debe a que nuestro
Workbench no está finalizado. En este momento, el sistema funciona en
gran medida a través de un reemplazo del shell AmigaDOS (o CLI para
los usuarios más viejos), aunque tenemos un Workbench y puedes navegar 
por los discos y lanzar las aplicaciones con él, las operaciones de
archivo no están completas. Los viejos programadores del Amiga deberían
leer "diferencias con la programación del Amiga" del capítulo 4 para
tener una visión de las diferencias.

Usando AROS
===========

AROS-alojado: ¿Un sistema operativo dentro de otro sistema operativo?
---------------------------------------------------------------------

AROS se desarrolló originalmente en Linux_ ejecutándose en una 
computadora basada en Intel. Aunque se ejecuta en muchas otras máquinas y Sistemas
Operativos. Esto puede sonar extraño: ¿Un OS funcionando encima de otro OS,
es eso emulación, correcto?

Un bonito término para lo que hace el AROS-alojado es "emulación de la API".
API es el acrónimo de tres letras de Interfaz del Programador de Aplicación.
En español a secas: una API proporciona funciones (en lenguaje C) que un
programador puede usar. La API AmigaOS consiste de un montón de llamadas de
biblioteca que un programador Amiga puede usar para hacer un programa Amiga.
AROS emula la API del AmigaOS: intenta proveer las mismas llamadas de 
biblioteca que AmigaOS. Un emulador de la Amiga, como UAE_; emula la *computadora*
Amiga: el procesador, el hardware conectado, todo. Esto tiene sus ventajas,
como ser capaz de ejecutar los binarios de los juegos Amiga en un hardware
diferente, y sus desventajas, como no ser capaz de usar el emulador como
un OS "real", sobre un procesador "real". AROS-alojado funciona sobre un
procesador "real". Pero no es un "real" OS, *a menos* que lo ejecutes de una
manera que no necesite Linux. Éste es el llamado AROS "nativo".

AROS puede funcionar de modo nativo en las computadoras Intel y Amiga, pero
no tan bien como funciona sobre Linux. Las funciones de biblioteca de AROS
se hacen para funcionar primero bajo Linux, usando internamente el núcleo
y las llamadas de biblioteca de Linux. De esta manera un programador tiene
la oportunidad para molestarse con los detalles técnicos en una etapa posterior.
En este momento hay personas que están trabajando en hacer el AROS "nativo"
más usable. Los resultados son muy impresionantes y es perfectamente posible
usar AROS-nativo como un real (y el único) Sistema Operativo en una máquina
compatible IBM PC.

Por supuesto, AROS no es *solamente* un emulador de API. También intenta
proporcionar reemplazos para todo el software de sistema del AmigaOS3.1,
y verás también unas pocas demos y juegos que son entregados con AROS, sólo
para mostar que funcionan - ¡podremos estar en un 77% del sistema completo,
pero ya tenemos a Quake andando!

Usando AROS "nativo" en i386
----------------------------
AROS nativo está en este momento bajo intenso desarrollo. Si quieres ver
trucos fenomenales, prueba AROS en Linux. Pero si (también) estás interesado en
el gran trabajo que los programadores han hecho, puedes probarlo
en modo "nativo".

Las instrucciones para instalar AROS nativo varían dependiendo de la plataforma
que uses. Debido a que "nativo" está en continuo desarrollo, los *resultados*
de instalarlo pueden variar según la edad del código que uses.

Sobre i386 hay diferentes medios de arranque. Primero y principal el
conjunto de binarios útiles es un LiveCD que puedes conseguir en la sección
de Descargas. Puede ser o una instantánea o una nightly build (la primera
es más estable pero anticuada, la última tiene los cambios más recientes
pero puede ser en casos raros inestable). Segundo es el disquete de arranque
AROS, que está previsto para arrancar los sistemas que son incapaces de
arrancar desde CD. Éste tiene un conjunto mínimo de características pero así
también un tamaño chico. Si no tienes una unidad de CD todavía se te puede
mostrar una parte de AROS.

Entonces, después de descargar el archivo AROS LiveCD desempácalo y graba
la imagen ISO a un CD-R(W). Si tu propósito es usar AROS en una máquina
virtual, puedes usar la ISO como un archivo. Una vez que el disco esté listo,
puedes reiniciar tu PC con el LiveCD. Si tu sistema no soporta arrancar desde
CD, también descarga y copia el disquete de arranque AROS a un disquete (con
Rawrite o Winimage, por ejemplo) y arranca desde éste, dejando el CD en su 
unidad (se ve sensacionalmente parecido a AmigaOS). Puedes pasear por el CD con
el Wanderer (o con el Shell), jugar algunos juegos/demos incluidos en los
programas contribuido en el CD, mirar en los fundamentos del sistema hasta que
te aburras. También es posible agregar archivos a una imagen ISO y conseguir
algún software extra escrito para AROS, y regrabar el LiveCD. Por ahora aquí
termina la parte simple de usar un AROS-nativo. Para probar todas las demás
características se necesita install_ el sistema al disco duro (real o virtual).
Este proceso no se puede llamar fácil, y debe ser tratado como experimental.
Está descripto en el Documento de Instalación. De cualquier modo, recuerda que
el trabajo sigue y pronto podrás tener más de AROS nativo - ¡manténte en contacto!

Usando AROS "nativo" i386 en máquinas virtuales
-----------------------------------------------
Actualmente las tecnologías de *Virtualización* están desarrolladas para un
reemplazo casi completo de una máquina real, han sido atizadas por las 
crecientes velocidades de las CPU.
Puedes hacer una máquina *virtual* dentro de tu sistema (el "anfitrión") y lanzar AROS
en ella, sin preocuparte por ninguna falla y relanzar rápidamente el sistema 
"invitado" si sucedió algo. 

Hay un número de paquetes de máquina virtual gratuitos, los más conocido son 
QEMU (gratuito, de fuente abierta, para la mayoría de los sistemas anfitrión),
VMWare Player (gratuito; también hay un completo servidor VMWare gratuito que 
requiere un número de serie que es gratis) y Microsoft VirtualPC (VPC) (gratis).

En vez de tener casi la misma configuración de AROS dentro de la MV, hay una
diferencia en la configuración de la MV misma.

VM para Linux/FreeBSD
"""""""""""""""""""""
QEMU en Linux es bastante fácil de configurar. Todo lo que se necesita es usar apt-get
en Debian/Ubuntu/Knoppix/DSL o usar cualquier otro administrador de
paquetes para las otras distribuciones o descargar y desempacar el archivo
manualmente. Puedes obtener el archivo del `sitio web de QEMU <http://fabrice.bellard.free.fr/qemu/>`__. 

También hay una MV VMWare disponible para Linux. Revisa el `sitio web de VMWare
<http://www.vmware.com>`__.

VM para Windows
"""""""""""""""

QEMU en Windoes es casi la misma cosa que en Linux. La diferencia está en la red
y en algunos otros temas. Puedes hallar información útil y los paquetes en
`la página de QEMU en Windows <http://www.h7.dion.ne.jp/~qemu-win/>`__ . 
También hay una bonita GUI para QEMU llamada QEMU Manager, que incluye el 
paquete QEMU.

QEMU debe ser lanzado como una aplicación de consola con algunos parámetros dados.
Más tarde revisaremos algunos de las opciones en otras secciones, lo que significa
que debes agregarlas a tu cadena de lanzamiento (o un guión).

.. Note::  

	QEMU es un virtualizador rápido, aunque su velocidad se puede acelerar más 
	instalando el módulo de núcleo KQEMU (y agregando la opción -kernel-kqemu si
	está en Windows). Pero recuerda que KQEMU puede hacer al sistema invitado
	inestable.
	No uses la combinación ALT+TAB para liberar el bloqueo del teclado, usa
	CTRL+ALT, de otro modo la tecla TAB puede quedar apretada y dañar el
	archivo editado recientemente.
    
Usar `VMWare <http://www.vmware.com/products/free_virtualization.html>`__
o VPC es incluso más fácil de configurar.
Todo lo que necesitas es instalar algún hardware virtual como la tarjeta de red
o de sonido y crear un HDD (unidad de disco rígido) virtual. Todo es 
administrado por una simple GUI.

VM para MacOS
"""""""""""""

Para las Macs PPC funcionando con el OS 9 o 10.X solamente está disponible 
el emulador de i386 `Virtual PC <http://www.microsoft.com/mac/products/virtualpc/virtualpc.aspx?pid=virtualpc>`__.
Pero no soporta a las Macs Intel. VPC es también un producto comercial caro. El
método alternativo para obtenerlo es comprando Office 2004 que viene con una copia
gratis de la versión más reciente (VPC 7). Advierte que el Mac VPC es esencialmente un
emulador, con una velocidad limitada y es demandante de una máquina PPC razonablemente
rápida (mira en el sitio web para más detalles).

Para las Macs Intel (OS X) QEMU ha sido transferido y renombrado como 
`Q <http://www.kju-app.org/kju/>`__ . Viene como un binario Intel y es gratis.
Q no soporta la virtualización directa aún (o al otro módulo de aceleración del
núcleo i386), haciendo que en el momento alcance sólo parte de la velocidad posible.

Otra opción (venidera) para la MV en Intel será el virtualizador `VMware Fusion`__,
esperado para comienzos del 2007. La versión beta 33141 ya soporta arrancar el 
AROS LiveCD, con la condición que esté deshabilitado el soporte de la unidad de disquete
en los parámetros de arranque de GRUB (sólo resalta tu selección en el menú de GRUB,
aprieta 'e' dos veces, agrega nofdc a la línea de comandos, aprieta return, después 'b'.
Si has instalado AROS en el HD (disco duro), puedes hacer el cambio permanente en el archivo
menu.lst).

__ http://www.vmware.com/whatsnew/macsignupform.html

Todavía otro producto de MV Mac Intel es Parallels, un producto comercial, aunque a un menor
costo que VPC. Por favor advierte que todavía falla en arrancar AROS. Lo mismo se aplica al menor
PC Parallels Workstation 2.1.

..  Note::  Los usuarios de las (primeras) portátiles Mac Intel que funcionan relativamente
			recalentadas pueden beneficiarse de usar la `SMC fan control utility`__.
			Permite ajustar la velocidad del ventilador para una mayor ventilación de tu
			máquina, manteniendo bajas las temperaturas durante las cargas de trabajo pesadas.
			Mientras que es considerado seguro para usar, ¡todavía considera los riesgos involucrados!

__ http://81.169.182.62/~eidac/software/page5/page5.html

Imágenes de discos virtuales
""""""""""""""""""""""""""""

Si has considerado instalar AROS al HD de la máquina virtual, puedes crear
el HDD virtual en QEMU usando el programa qemu-img (reemplaza <tamaño> con
el tamaño necesario en bytes, M o G para Mega o Giga) con el comando::
    
    qemu-img create -f qcow aros.img <size>

QEMU Winaros está `aquí <http://amidevcpp.amiga-world.de/WinAros/WinAros_Light_QEMU.zip>`__ 
y VirtualPC `aquí <http://amidevcpp.amiga-world.de/WinAros/WinAros_Light_VPC.zip>`__ .
Un conjunto de imágenes de disco pre-instaladas con AROS o vacías está disponible para hacer
la ejecución de AROS en una MV más fácil. WinAros es un ambiente pre-instalado de AROS
en una imagen de HD, compatible con las famosas máquinas virtual de QEMU y 
Microsoft VirtualPC, ambas disponibles gratuitamente en la red. Puedes descargar
ambas versiones de WinAros del `sitio web <http://amidevcpp.amiga-world.de/afa_binarie_upload.php>`__.

El Kit de Instalación para AROS (IKAROS) es un conjunto de imágenes de disco virtual
para los diferentes virtualizadores, incluyendo QEMU y VMWare, ya particionadas,
formateadas, y listas para instalar AROS. Sus beneficios son el pequeño tamaño
de archivo, pues no incluyen una gran cantidad de archivos, y la posibilidad para
instalar versiones frescas de AROS, que lo hacen útil para probar las nightly
builds. Permite la instalación fácil de las nuevas versiones sin enredarse
con la configuración de la partición. Están incluidas las instrucciones de
instalación. Por favor revisa `los Archives de Aros-Exec <http://archives.aros-exec.org/index.php?function=browse&cat=emulation/misc>`__ 
en la sección (emu/misc) para las puestas al día.

Usando el AfA en m68k
---------------------
En una Amiga (m68K), puedes poner el código nativo en alguna parte de
tu disco rígido, apretar dos veces en el ícono "boot", hacer un reset 
y disfrutar de un completo sistema Amiga. Esto se debe a que no es *realmente*
nativo. El programa de arranque sólo reemplaza temporalmente una pocas
bibliotecas del AmigaOS con las de AROS. Por supuesto que esto es bueno
para el propósito de probar, pero al final todavía ejecutas el bueno y viejo
AmigaOS y no AROS nativo. Esto cambiará a medida que construyamos un sistema
AROS 68k más completo. Este sistema es llamando AfA (AROS para Amigas).


Usando AROS alojado en Linux o en FreeBSD
-----------------------------------------


Una vez que conseguiste los binarios para tu sistema, ya sea compilándolos
o descargando los pre-compilados, deberías ir al directorio "bin/$TARGET/AROS",
donde $TARGET es tu sistema (algo como "linux-i386"). Ejecuta el archivo
"aros" ("./aros"). El reemplazo del Workbench, "Wanderer", se iniciará. 

Hay unas opciones de línea de comandos para el ejecutable de aros que se podrían
usar. Puedes obtener la lista con la opción ./aros -h.

Falta agregar...

Ya que "Wanderer" es muy limitado preferirás trabajar con el Shell.
Puedes iniciarlo desde el menú "Wanderer/Shell". Ahora deberías tipear los comandos,
y el más importante es "dir": te mostrará los contenidos del directorio.
El directorio llamado "C" contiene todos los comandos, por lo que sería útil
mostrar su contenido con "dir c:". El Shell se comporta como el shell AmigaDOS,
y los comandos en "C" se comportan como sus equivalentes de AmigaDOS. (Nota
para la gente de UNIX: para dirigirse al directorio padre usen "/" y no "..":
esto se verá feo porque AROS piensa que el ".." de Linux es un directorio normal.
No deberían usar "./" como el prefijo para dirigir un comando al interior del directorio
actual, sino dejarlo vacío). Una vez que lo uses, prueba ejecutar algunos
programas (especialmente los "Demos" y "Games") para tener una impresión
de las capacidades de AROS.

Usando AROS alojado en PPC
--------------------------

Queda para que alguine lo escriba...


Los fundamentos de AROS
=======================

Lo básico de la GUI Zune de AROS
--------------------------------

La abreviatura GUI significa Graphical User Interface (Interface/icie Gráfica de Usuario),
y se aplica a todos los medios usados por el OS para interactuar con el usuario 
de un modo diferente a la interface de línea de comandos (CLI). Para aquellos 
que nunca han usado algún OS de la marca Amiga, será útil darles algunos fundamentos
de GUI para ayudarlos a usar nuestro sistema. No obstante, algo será específico de AROS.

Un sistema Amiga usa principios definidos y comunes, como ya habrás notado.
Primero, cualquier opción de menú de la ventana de cualquier aplicación no está
anexada a esa ventana - se movió a la franja superior, donde se puede
acceder con facilidad. Para hacerlo, selecciona la ventana que necesites, mueve
el puntero del ratón al extremo superior de la pantalla. Entonces, si presionas
el botón derecho allí, puedes ver un menú descender, representando las opciones
de nuestra aplicación. Sí, se ve como el MacOS. También puedes hacer que el menú
aparezca en cualquier lugar de la pantalla, en donde presiones el botón izquierdo
del ratón. Para hacer esto, ... Por ejemplo, si ninguna ventana de aplicación 
es seleccionada, entonces podrás ver el menú del Wanderer.

Ahora, consideremos a nuestro escritorio - como ya probablemente sabrás, se llama
Wanderer. ¿Qué es? Bueno, Wanderer es una aplicación, igual que cualquier otra.
De hecho, es el administrador de archivos de AROS, que te permite escoger y
operar con archivos (todavía la función no está completa), lanzar programas,
obtener información del sistema, lanzar el CLI (la ventana de comandos) y otras
funciones. Por lo general, se abre a lo ancho de la pantalla y actúa como tu 
escritorio (los íconos en el escritorio representan los volúmenens y los discos
con los que puedes trabajar). Puede ser puesto a un lado desmarcando la opción
Backdrop (Telón), que se halla en el menú del Wanderer (¿recuerdan en el párrafo previo?).
Después de eso, el Wanderer se convierte en otra ventana que puedes mover, cambiar
de tamaño, etc. Así que, puedes ver, no es como el escritorio de Windows o de otro
OS fijo en su lugar. Por supuesto, incluso puedes no usar el Wanderer y usar tu
administrador de archivos preferido (p.e. Directory Opus).

Pero, ¿cómo se comportarán entonces las aplicaciones? ¿En dónde se abrirán las 
ventanas? Está el término `pantalla` -una pantalla es el lugar en donde tu 
ventana está preparada para abrirse. Dicho esto, si la aplicación va a abrirse 
en la pantalla del Wanderer, se verá como usualmente pasa en otro OS, tu app 
aparecerá como una ventana en el escritorio. Por otra parte, la ventana puede
abrirse en una pantalla propia- se ve como que captura la pantalla entera. Pero
puedes intercambiar las pantallas con un gadget en la esquina superior derecha
de la pantalla (esto también se aplica a una sola ventana). Así que puedes
intercambiar entre Wanderer, Directory Opus y cualquier otra app abierta en su
propia pantalla. Este comportamiento viene de la historia del Amiga.

Bien, ha llegado la hora de decir algo sobre las ventanas. Una ventana de AROS
tiene botones de control para manipularla, llamados gadgets (que se pueden
traducir como un tipo interactivo de elemento gráfico). Primero, el de la esquina
superior izquierda de una ventana permite cerrarla. El siguiente en la parte derecha
permite minimizar/maximizar la ventana. Y el restante se usa para poner la ventana
atrás o adelante igual que como intercambiamos entre las pantallas. Las ventanas
pueden no tener gadgets (se ven como la demo Kitty -que incluso no tiene bordes
aunque todavía conserva una forma bien curvada) o tener un conjunto diferente.

Los contenidos de la ventana consisten de algunos elementos usuales vistos en 
cualquier GUI - botones, listas, cadenas de texto, cualquier otro tipo de gadget.
Si una aplicación tiene previsto cambiar cualquier preferencia del sistema o de una
aplicación lo usual es llamarla *Pref* y tiene su conjunto de botones para operar.
Por lo general, estos botones son: TEST (aplica todos los cambios hechos por Pref
pero no guarda los cambios, y cierra la ventana), SAVE (guarda 
los cambios y cierra la ventana), USE (aplica los cambios y cierra la ventana, 
pero no los guarda), CANCEL (descarta todos los cambios y cierra la ventana).

También, de la historia de Amiga la unidad de colocación de archivos se llama
cajón en vez del carpeta/directorio de otros sistemas, pero su significado
es el mismo. Tradúcelo como directorio si estás inseguro.

Como en el original Amiga, AROS dispone de teclas especiales, usadas para ejecutar
comandos rápidos. Las teclas Windows izquierda y derecha (en el teclado de PC) 
reemplazan a las teclas Amiga originales y se usan en diferentes combinacions para
lanzar comandos.

Otro nombre desconocido que puedes encontrar en AROS es Zune. ¿Qué es Zune? Es la
biblioteca de GUI desarrollada para reemplazar en la mejor tradición a MUI (Magic
USER Interface), ampliamente usada en Amiga. ¿Pero hay una aplicación llamada
Zune? Puedes hallar Zune Pref que te permite establecer las configuraciones para
las aplicaciones basadas en Zune en conjunto o individualmente. Por ejemplo, 
para establecer las preferencias de Zune para Wanderer puedes seleccionar
GUI prefs de su menú, o para establecer las prefs de Zune para otras apps 
puedes usarlo como el comando del CLI Zune <nombre de la aplicación>.

Falta para terminar...


AROS CLI (Command Line Interface)
---------------------------------

Falta - Un resumen y comparación de los comandos del CLI...

AROS tiene su CLI, la Interface de Línea de Comandos, que expande en gran
medida las capacidades del OS. Quienes usaron el AmigaOS pueden notar que se
ve muy parecido al AmigaDOS. Hay algunos fundamentos del CLI descriptos en
`introducción <shell/introduction>`__ para los comandos del CLI. 

En este momento no necesitas tipear todos los comandos al final - ahora hay
un ingeniosa finalización con el tabulador similar a la de las consolas linux.
Esto también te permite agregar los nombres de archivo o escogerlos de una lista.

Falta terminar...

Los programas de sistema de AROS
--------------------------------

Hemos mencionado a las aplicaciones, así que es bueno dar una descripción  de
sus funciones. Entonces, hay grupos de aplicaciones de sistema de AROS reunidas
en directorios separados:

	+ C - el lugar de todos los comandos del sistema usados en el CLI.
	+ Classes - el lugar de los datatypes, imágenes de gadget y las clases de
	            Zune.
	+ Devs - donde los archivos relacionados con dispositivos (controladores,
	         keymaps) y los datatypes están ubicados.
	+ Extras - donde residen todos los programas contribuidos.
	+ Fonts - aquí puedes hallar todas las fuentes del sistema. Cualquier fuente
              adicional debe ser agregada (asignada) a este directorio.
	+ Libs - donde están ubicadas las bibliotecas del sistema.
	+ Locale - guarda los archivos de catálogo de las traducciones de las 
		       apps de AROS.
	+ Prefs - tiene los programas de edición de las preferencias.
	+ S - contiene algunos guiones de tiempo-de-lanzamiento del sistema.
	+ System - el lugar para algunos controles del sistema.
	+ Tools - el lugar de algunas apps del sistema usadas comúnmente.
	+ Utilities - el lugar de algunas apps no tan usadas pero todavía útiles.

En vez de aplicaciones, hay otros programas de ejecución permanente llamados
*tasks*.

Otro tipo de aplicaciones AROS son los *Commodities*. Éstas son aplicaciones que
pueden ayudarte a hacer tu sistema más cómodo. Por ejemplo, las ventanas de AROS no
se ponen delante de las otras cuando tú aprietas en ellas, y puedes encontrarlo
incómodo. Para acomodar esa comportamiento puedes usar el commodity de AROS 
ClickToFront. Se lo puede encontrar debajo de los otras commodities en el directorio 
SYS:Tools/Commodities. Cuando aprietas dos veces en él, la ventana se pondrá 
encima de las otras si es dos veces apretada. Otro ejemplo es el commodity
Opaque - que te permite mover las ventanas con sus contenidos. También está 
el commodity Exchange que te permite manipular los commodities lanzados 
y tener información sobre ellos. Por lo general los commodities no abren 
ninguna ventana (propia).

Para operar con archivos de diferentes tipos los sistemas como Amiga usan los
*datatypes*. Un datatype es un tipo de biblioteca del sistema que 
permite a los programa leer y/o escribir a tales archivos sin conocer la
implementación del formato.

Y si cavamos un poco más profundo hay algunos términos del sistema que se
pueden explicar. AROS usa *handlers* para comunicarse con los sistemas de 
archivo y *HIDD* para comunicarse con el hardware.

Falta terminar...

Ajustando el AROS instalado
===========================

Configurando el Locale
----------------------

AROS se está convirtiendo en un sistema verdaderamente internacional en
estos días, siendo traducido a muchos idiomas. La traducción no es muy
difícil, y el número de los traductores de AROS se está incrementando.
Cuando el soporte para Unicode sea implementado, podrá ser traducido a
cada idioma usado.
Si sientes que puedes darle AROS a tu país, tanto el OS como la documentación,
no dudes en ponerte en contacto con nosotros y ofrecer tu ayuda.

Sobre el idioma. Primero, dependiendo de las fuentes usadas debes establecer
las fuentes lanzando SYS:Pres/Fonts y designar Fonts para diferentes textos
del sistema, Icons (usada para las etiquetas de los íconos), Screen 
(usada en la pantalla común) y System (usada en la ventana del CLI). Si tu
idioma usa un conjunto diferente de la ISO (por ejemplo, cirílico CP-1251)
las fuentes *deben* estar en el código de página correcto. Actualmente AROS
puede usar dos tipos de fuentes - las fuentes bitmap de Amiga (que se
pueden usar directamente) y TrueType (a través del administrador FreeType 2,
que todavía tiene algunas cuestiones con los códigos de página no ISO).
Las fuentes bitmap son de algún código de página en particular, y las TTF
pueden ser Unicode. 

¿Cómo puedes cambiar el locale de AROS? Necesitas lanzar el pref Locale 
en SYS:Prefs. Allí puedes ver una lista de los locales soportados y
seleccionar tu preferido. En la segunda página de este Pref puedes seleccionar
el país usado (esto da el formato correcto para la moneda y la fecha/hora).
Y la última pestaña te permite cambiar la zona horaria a la que se usa en tu ubicación.

Después que has hecho los cambios a las fuentes reinicia el sistema, y debes
poder de ver todo el contenido traducido.

Así que ahora podemos leer, pero ¿podemos escribir en nuestro idioma?. Para
hacerlo debes cambiar la distribución del teclado.

Las configuraciones del teclado y del ratón son administradas por el pref
Input. Puedes cambiar la distribución y apretar *Use* aunque podemos hacerlo
mejor. Esta herramienta también te permite guardar los preajustes - igual
que cualquier aplicación tiene un menú, te permite guardar tus preferencias 
en un archivo con el nombre dado y mantener diferentes configuraciones de 
locales. Esto último es lo que usaremos para conmutar nuestras distribuciones de 
teclado. Escoge tu distribución de teclado de la lista y aprieta el botón
izquierdo para abrir el menú contextual. Después ingresa el nombre de tu
preajuste a la cadena File, digamos, *locale1* y aprieta Ok para guardarlo
en el directorio SYS:Prefs/Presets. Ahora escoge la distribución American (PC)
y repite el guardado de preajustes, digamos, con el nombre *english*. Estos
preajustes se pueden usar más tarde para conmutar las distribuciones. Aprieta
*Cancel* para salir.

Está el commodity FKey que te permite asignar acciones a algunas
combinaciones de teclas. Ahora lancémoslo y asignemos la conmutación del
locale. Después que hagas doble click en el ícono de FKey, se lanza el
Exchange, escoge la FKey de la lista y aprieta el botón *Show*. Esto invocará
la ventana FKey. Puedes ver en la lista que ALT TAB está asignado a la conmutación
de ventanas. Ahora aprieta enter en la primera combinación, digamos, *ALT Z* y ve al 
panel derecho. Escoge *Launch the program* del menú descendente e ingresa como
argumento SYS:Prefs/Input. Anexa el switch USE y el nombre del preajuste
*english* a la cadena como se muestra::

    SYS:Prefs/Input USE SYS:Prefs/Presets/english

Aprieta el botón *New* para agregar la otra combinación. Ahora establece
la combinación para tu locale como se mostró arriba, reemplazando el nombre
*english* con el nombre de tu preajuste. Aprieta de nuevo el botón *New* y
después *Save Settings*. Ya puedes usar las combinaciones definidas para 
conmutar entre las distribuciones.

Instalando el software
----------------------

En este momento no hay un sistema instalador en AROS. Instalar una aplicación
por lo general significa que tienes que extraerla en algún directorio en la
unidad de disco duro o en el ramdisk. Después algunos programas requieren que hagas
asignaciones, lo que se hace con el comando Assign en el CLI, y algunas
adiciones a los guiones de inicio.
Por ejemplo, Lunapaint necesita que Lunapaint: sea asignado al directorio
en que fue extraído para que funcione correctamente. Puedes hacerlo con el
comando 

    Assign Lunapaint: Disk:Path/Lunapaint

Pero si no quieres tipear este comando después de reiniciar para lanzarlo 
de nuevo, debes ponerlo en el guión S:User-Startup.
Para hacer esto, tipea este comando en el prompt del CLI::

    :> edit SYS:S/User-Startup
    
Después inserta la asignación de Lunapaint (u otro programa) al final del
archivo. Guarda los cambios y lo tendrás arreglado.

Otra manera es usar el directorio ENVARC:SYS/Packages. Todo lo que necesitas
es crear un archivo de texto con el nombre de tu aplicación y poner la ruta
a tu aplicación en ese archivo. Después crea un directorio llamado S en el 
directorio del programa y pon el archivo package-startup allí. Esta manera es
más segura, pero puede no serte tanto del estilo Amiga.


Configurando la red
-------------------

Para comunicarse con otras computadoras en una red, AROS usa una pila TCP, la
AROSTCP, que es un puerto del AmiTCP. Este software está ubicado en el directorio
/Extras/Networking/Stacks/AROSTCP. Configurarlo no es fácil pero está en 
desarrollo un tipo de herramienta GUI. Además entérate de que hay muy pocos
programas de red en AROS (pero algunas interesantes herramientas están en
desarrollo para ser presentadas pronto).

Primero, necesitas configurar el lado de tu máquina de la red. Esta parte puede
diferir dependiendo de tu hardware. En una máquina real necesitas instalar la
tarjeta de red (NIC) soportada y enchufar el cable. En una máquina virtual debes
configurar la implementación de la NIC y revisar si está soportada por AROS
(al menos, las de QEMU y VMWare lo están).

La red en QEMU/Linux
""""""""""""""""""""

Lee los consejos para lanzar AROS sobre Linux QEMU anteriores.

Después que esto esté habilitado podemos ir al siguiente punto.

La segunda parte es configurar AROSTCP en AROS para que funcione.

En los sistemas linux se necesitan algunos pasos para hacer que la red en la
máquina virtual funcione.

Se debe cargar el módulo tun (túnel):: 

    #> modprobe tun

Después, el núcleo debe convertirse en un ruter::

    #> echo 1 > /proc/sys/net/ipv4/ip_forward

Luego se debe agregar una regla al firewall::

    #> iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE

Para terminar, todavía siendo root, inicia Qemu con::

    #> qemu -cdrom aros.iso -m 48

El módulo tun de Linux, por omisión, crea un gateway para la red sintética
en 172.20.0.0/16 con un gateway en 172.20.0.1.
Digamos que nuestra máquina alojada en Qemu está en 172.20.0.10.
Digamos que tu LAN es 192.168.0.0/24 con un DNS en 192.168.0.1 (o en
cualquier parte en Internet, no importa).

*Para QEMU en Windows la red en modo usuario debe ser reemplazada con 10.0.2.16
para el anfitrión y 10.0.2.2 para el gateway, o usar el adaptador TAP,
que es mejor. Recuerda configurar tu firewall para que los paquetes de 
QEMU puedan pasar.*

Tienes que editar tres archivos en el cajón SYS:extras/Networking/stacks/AROSTCP/db:
En *hosts* quita o comenta cualquier entrada. Por ahora los anfitriones estarán
hosts, interfaces y netdb-myhost.
en netdb-myhost.
En *interfaces* quita el comentario al renglón prm-rtl8029.device (QEMU está
emulando esta NIC entre otras, puedes usar pcnet32.device para VMWare), edítalo
(cambia una cadena *IP=* a la que estaba arriba)::

    eth0 DEV=DEVS:networks/prm-rtl8029.device UNIT=0 NOTRACKING IP=172.20.0.10 UP

En *netdb-myhost*, agrega los anfitriones locales conocidos, tu nombre de 
dominio, el gatewary::

    HOST 172.20.0.10 arosbox.lan arosbox
    HOST 172.20.0.1 gateway
    DOMAIN lan
    NAMESERVER 192.168.0.1

El directorio db puede residir en cualquier parte, pon su ruta en el archivo
ENVARC:AROSTCP/Config, te aconsejo que copies los archivos db en el directorio
(creado) ENVARC:AROSTCP/db, de ese modo el archivo Config sería::

    ENV:AROSTCP/db

Ahora haz que AROSTCP se inicie al arrancar con la palabra "True" en
ENVARC:AROSTCP/Autorun (Si no existe crea el archivo en la ventana del CLI
con el comando 
echo "True" >sys:AROSTCP/Autorun)
Edita SYS:Extras/Networking/Stacks/AROSTCP/S/Package-Startup::

    ; $VER: AROSTCP-PackageStartup 1.0 (01/08/06)
    ; AROSTCP-PackageStartup (c) The AROS Dev Team.
    ;
    Path "C" "S" ADD QUIET

    If not exists T:Syslog
        makedir T:Syslog
    Endif

    If not exists EMU:
        if $AROSTCP/AutoRun eq "True"
        C:execute S/startnet
        EndIf
    EndIf

El archivo Sys:extras/Networking/Stacks/AROSTCP/S/Startnet debería ser
algo así::

    ; $VER: AROSTCP-startnet 1.0 (01/08/06)
    ; AROSTCP-startnet (c) The AROS Dev Team.
    ;
    Run <NIL: >NIL: AROSTCP
    WaitForPort AROSTCP
    If NOT Warn
        run >NIL: route add default gateway
    Else
    ; echo "Wait for Stack Failed"
    EndIf

Después del arranque, prueba con::

    ifconfig -a

Debes ver una salida como esto::
    
    lo0: flags=8<LOOPBACK> mtu 1536
            inet 0.0.0.0 netmask 0x0
    eth0: flags=863<UP,BROADCAST,NOTRAILERS,RUNNING,SIMPLEX> mtu 1500
            address: 52:54:00:12:34:56
            inet 172.20.0.10 netmask 0xff000000 broadcast 172.255.255.255

Si puedes ver la cadena eth0 entonces tu interface de red está levantada. Puedes
probarla lanzando estos comandos::

    AROS:>ping 172.20.0.1
    PING 172.20.0.1 (172.20.0.1): 56 data bytes
    64 bytes from 172.20.0.1: icmp_seq=0 ttl=255 time=xx ms
    64 bytes from 172.20.0.1: icmp_seq=1 ttl=255 time=xx ms
    64 bytes from 172.20.0.1: icmp_seq=2 ttl=255 time=xx ms
    
    --- 172.20.0.1 ping statistics ---
    3 packets transmitted, 3 packets received, 0% packets loss
    round trip min/avg/max = x/xx/xx ms

Una salida como ésta significa que los paquetes de nuestra interface 
alcanzaron el gateway en la dirección 172.20.0.1. Si obtienes los errores
de Host unreachable, entonces revisa tus configuraciones AROSTCP y las opciones
de la máquina virtual.

En Windows: Para hacer una red externa accesible a la máquina virtual debes
configurar el ruteo desde nuestra red virtual a una real, como hacer
que un sistema anfitrión funcione como un router. Para Linux esto ya está hecho.

Puedes hacer incluso más pruebas haciendo ping a los otros anfitriones e
intentar usar las aplicaciones de red que puedes encontrar en Archives.aros-exec.org,
como ftp y AIRCos. Si usas un programa FTP con tu servidor FTP, recuerda
que solamente funciona con servidores ftp pasivos, y configura tu servidor
de este modo.


La red en QEMU/Windows
""""""""""""""""""""""

Configurar QEMU para que funcione en Windows es relativamente más difícil que
para Linux. Primero, asegúrate que pusiste tu Firewall en el modo aprendizaje
(o prepáralo para recibir nuevas reglas) o deshabilítalo por completo. El 
Firewall puede bloquear las transferencias a la MV.

Hay dos maneras para usar la red con QEMU sobre Windows. Primero y lo más
probado es usar la interface tap. Para usarla debes descargar el paquete 
de `OpenVPN <http://openvpn.net>`__ 2.0 para Windows (solamente XP). Después de
que lo instales, tendrás una conexión de red extra en estado de desconexión.
Cámbiale el nombre a, digamos, eth0. Después ve a las propiedad de la 
conexión eth0 y pon una dirección IP en las propiedades del protocolo TCP-IP.
Debes poner la dirección IP en *otra* subred diferente de tu IP base (si 
tienes una 192.168.0.x, entonces pon, digamos, la misma 10.0.2.2) y la
máscara de red 255.255.255.0. *Reinicia*. Después reemplaza las opciones 
del renglón de inicio en QEMU (o agrégalas si no estaban) -net nic -net tap,
ifname=eth0. Luego pon AROS como se describió arriba para la red en modo 
usuario. Fíjate que necesitarás los privilegios de administrador para instalar
el adaptador TAP OpenVPN.

La segunda opción es usar una pila de red de modo usuario que se lance por 
omisión (o usar los switches "-net nic -net user", que ahora están predeterminados).
Las opciones dadas son para la versión 0.8 o superior de QEMU. La configuración del 
lado AROS es similar a la que se usó en Linux pero necesitarás usar la siguiente
dirección IP para configurar y probar: 10.0.2.16 para la IP de la máquina
AROS (en vez de 172.20.0.10), 10.0.2.2 para el gateway (en vez de 
172.20.0.1). Este modo puede funcionar sin que el usuario tenga los privilegios
administrativos, pero *puede hacer que algunas aplicaciones AROS dejen de 
funcionar apropiadamente (como el cliente FTP)*.

Hay disponibles algunas guías en como configurar la red de QEMU en Windows:

    + Para `VLan <http://www.h7.dion.ne.jp/~qemu-win/HowToNetwork-en.html>`__
    + Para `Tap <http://www.h7.dion.ne.jp/~qemu-win/TapWin32-en.html>`__

La red en VMWare
"""""""""""""""" 

La red del lado de VMWare es relativamente fácil de configurar. Todo lo que
necesitas es agregar la NIC a la configuración de tu MV y asignar la IP a la 
nueva conexión de red, asociada con esa tarjeta. Las otras notas de uso son 
las mismas que para QEMU, excepto para el tipo de adaptador en el archivo
en SYS:Extras/Networking/Stacks/AROSTCP/db/interfaces ::

    eth0 DEV=DEVS:networks/pcnet32.device UNIT=0 IP=10.0.2.2 UP

La red en una PC real
"""""""""""""""""""""

En una PC real necesitarás hacer todo lo que puedes hacer para cualquier OS 
-preparar el hardware a conectar en la caja AROS -cables, concentrador y demás.
Después debes configurar el lado AROS del mismo modo que arriba, reemplazando las 
direcciones IP por las aceptables para tu LAN para una caja AROS, el gateway
y el DNS. Configurar la tarjeta de red en el archivo *interfaces* quitando el 
comentario al renglón que corresponde a tu tarjeta.

Por terminar...

Configurando el sonido
----------------------

Actualmente no hay mucho de sonido en AROS. Primero, en este momento no hay
controladores funcionando para las tarjetas de sonido implementadas en la 
máquina virtual (la usual sb16/es) así que la única maneta de probrar obtener
sonido es usar AROS-nativo en una pc con una tarjeta real SB Live/Audigy.
También están soportados los codecs que siguen la norma AC97.

El sonido AHI en AROS también soporta las opciones sin sonido (VOID) y de
escritura en disco.

Queda para que alguien siga escribiendo...

¿Ésa es toda la información del usuario en esta guía?
=====================================================

Este capítulo debería haberte contado cómo conseguir, instalar y usar AROS.
Después de haber probado ejecutar cada programa en los directorios C, Demos,
Utilites, Tools, Games, etc., te podrías extrañár si eso es todo. ¡Sí,
eso es todo lo que un "Usuario" puede hacer con AROS! Pero cuando algún 
nuevo código importante de usuario esté listo, por supuesto será agregado
a esta guía.

Si crees que no te proporcioné suficiente información sobre la compilación,
la instalación, Subversion, el shell, etc., podría ser bueno que sepas
que tengo razones para eso. Primero, ya hay mucha información disponible, 
y sería innecesario además de injusto sólo copiar esa información en este
documento. Segundo, estamos hablando de información muy particular. Alguno
de los lectores podría estar interesado en compilar el código fuente, otros
podrían querer saber todo acerca del shell de Amiga. Entonces, para mantener
legible esta guía, solamente dirigo a los lugares donde puedes encontrar esa
información, en vez de proporcionarla aquí. Tú, el lector, puedes entonces 
decidir eso es de tu interés.


.. _Linux: http://www.linux.org/
.. _UAE:   http://www.freiburg.linux.de/~uae/
.. _install: installation

