================================
Guía para la instalación de AROS
================================

:Authors:   Stefan Rieken, Matt Parsons, Adam Chodorowski, Neil Cafferkey, Sergey Mineychev
:Copyright: Copyright ² 1995-2006, The AROS Development Team
:Version:   $Revision: 25677 $
:Date:      $Date: 2007-04-07 07:21:50 +1000 (Sat, 07 Apr 2007) $
:Status:    Needs to be updated for some AROS ports. Can be translated.
:Abstract:
    Este manual te guiará por los pasos necesarios para instalar los 
    diferentes sabores de AROS.
    .. Warning:: 
    
        AROS es un software de calidad alfa. Esto significa que en este momento
        es divertido para jugar y cool para desarrollar. Si estás aquí porque
        piensas que AROS es un sistema operativo terminado, completo y totalmente
        usable, es probable que estés decepcionado. AROS todavía no lo es, pero nos
        estamos moviendo lentamente en la dirección correcta.


.. Contents::


Descargas
=========

AROS está bajo intenso desarrollo. El resultado es que tienes que escoger entre 
estabilidad y características. En la actualidad hay dos tipos de paquetes binarios
disponibles para descargar: las instantáneas y las nightly builds.

Las instantáneas se hacen a mano con poca frecuencia, sobre todo cuando hay
una gran cantidad de cambios útiles en AROS desde la última instantánea y alguien
se siente motivado para crear una nueva instantánea. En fin, no hay un cronograma
de entregas regular. Incluso aunque se hacen muy de vez en cuando y que intentamos
escoger las veces en que AROS es particularmente estable, no hay garantías de que estarán
sin errores o funcionaran en alguna máquina en particular. Dicho esto, nosotros 
intentamos probar las instantáneas en una amplia variedad de máquinas, así que en la
práctica deberían funcionar relativamente bien.

Las nightly builds se hacen, como su nombre indica, cada noche de un modo automático
directamente desde el árbol de Subversion y contiene el código más reciente. Sin embargo,
no se prueban de ninguna manera y pueden estar horriblemente rotas, extremadamente 
defectuosas y pueden hasta destruir tu sistema en el caso que no seas afortunado. Aunque
la mayoría de las veces funcionan bastante bien.

Por favor mira en la `página de descargas`_ para tener más información sobre qué 
instantáneas y nightly builds están listas y cómo descargarlas.


Instalación
===========

AROS/i386-linux y AROS/i386-freebsd
-------------------------------------

Requerimientos
""""""""""""""

Para ejecutar AROS/i386-linux o AROS/i386-freebsd necesitarás lo siguiente:

+ Una instalación FreeBSD 5.x o Linux que funcione (no importa qué distribución
  uses, mientras que sea reciente).
+ Un servidor X configurado y funcionando (por ejemplo X.Org o XFree86).

Eso es todo.


Extracción
""""""""""

Dado que AROS/i386-linux y AROS/i386-freebsd son sabores hosted de AROS,
la instalación es simple. Consigure los archivos apropiados para tu plataforma
de la `página de descargas`_ y extráelos dónde quieras::

    > tar -vxjf AROS-<version>-i386-<platform>-system.tar.bz2

Si descargaste el archivo contrib, tal vez quieras también extraerlo (aunque
ahora su contenido ya está incluido en el archivo system y en el LiveCD)::

    > tar -vxjf AROS-<version>-i386-all-contrib.tar.bz2


Ejecución
"""""""""

Después de extraer todos los archivos puedes lanzar AROS así::

    > cd AROS
    > ./aros


.. Note:: 
    
    A menos que estés ejecutando XFree86 3.x o anterior, puedes notar que la 
    ventana AROS no se refresca de un modo apropiado (por ejemplo cuando una
    ventana diferente pasa encima de ella). Esto se debe al hecho que AROS usa la
    funcionalidad de X "backingstore" que por omisión está desactivada en XFree 4.0
    y posterior. Actívala, y agrega el siguiente renglón a la sección dispositivo
    de tu tarjeta gráfica en tu archivo de configuración de X (por lo común nombrado
    ``/etc/X11/xorg.conf``, ``/etc/X11/XF86Config-4`` or
    ``/etc/X11/XF86Config``)::

        Option "backingstore"

    Una sección dispositivo completa podría verse algo así::

        Section "Device"
            Identifier      "Matrox G450"
            Driver          "mga"
            BusID           "PCI:1:0:0"
            Option          "backingstore"
        EndSection


AROS/i386-pc
------------

Requirimientos
""""""""""""""

Necesitarás una PC promedio (basada en i486 o Pentium) con un *ratón PS/2* y un teclado AT o PS/2,
un disco duro y unidad de CDROM ambas IDE, una tarjeta de video (S)VGA y un monitor. También se 
puede usar, en vez de todo lo anterior, una VM (máquina virtual) compatible con una PC.
Se recomienda al menos que la tarjeta gráfica VGA sea compatible con la norma VESA y tenga 
16 MB de Ram de video.
AROS dispone de controladores acelerados genéricos (HIDD) para las tarjeta de ATI y de nVidia.
También puedes agregar una tarjeta de interface de red (hay algunas que están soportadas) para
probar la funcionalidad de red de AROS.
En caso de problemas revisa el FAQ para información relativa a tu tipo de hardware.


.. Note:: 
    
    Todavía no recomendamos la instalación de AROS/i386-pc en el disco duro [#]_  Aunque
    necesitarás sí o sí instalar AROS para probar algunas de sus características y workarounds. 
    Por favor fíjate que **no deberìas** usar install en tu máquina de trabajo,
    ¡cuyo disco duro contiene los valiosos datos!
    No estamos asumiendo alguna responsabilidad por cualquier pérdida de datos ocurrida
    durante la instalación. Es bienvenido cualquier informe sobre errores en la instalación.

Medios de instalación
"""""""""""""""""""""

El medio recomendado de instalación para AROS/i386-pc es el CDROM, porque podemos
incluir el sistema entero en un único disco (y también todo el software contribuído).
Además hace que la instalación sea más fácil, ya que no tienes que pasar por los 
cambios de disquete para transferir el software.

Como nadie en este momento vende CDROM con AROS (o en cualquier otro medio),
necesitarás acceder a una grabadora de CD para crearte el disco por tí mismo.


CDROM
^^^^^

La grabación
''''''''''''

Simplemente descarga la image ISO de la `página de descargas`_ y grábala a un CD
con tu programa favorito de grabación. Hay programas gratuitos de grabación de CD
para cualquier sistema, y para los usuarios de Windows podemos recomendar 
`InfraRecorder <http://infrarecorder.sourceforge.net>`__ - es gratuito, pequeño y
rápido, un simple Nero-killing.


El arranque
'''''''''''

La manera más simple para arrancar desde el CD de instalación de AROS es si tienes una
computadora que pueda arrancar desde CDROM. Para esto podría ser necesario habilitar el
arranque desde CDROM en la configuración del BIOS ya que a menudo está deshabilitado.
Simplemente pon el CD en la primera unidad de CDROM y reinicia la computadora. El arranque
es automático, y si todo va bien deberías ver una agradable pantalla después de una cierta
espera.

Si tu computadora no soporta arrancar desde CDROM, entonces puedes crear un disquete_
de arranque y usarlo junto con el CDROM. Simplemente inserta ambos en sus unidades y
reinicia. AROS arrancará desde el disquete, pero después de que las cosas importantes 
se hayan cargado en memoria (incluyendo el manejador del sistema de archivo de CDROM), 
continuará el proceso desde el CDROM.


Disquete
^^^^^^^^

Hoy en día los disquetes solamente pueden ser útiles para arrancar la computadora si
el BIOS no soporta el arranque desde CDROM o en algunas PC realmente obsoletas. Así
que todavía se los mantiene.


La grabación
'''''''''''''

Para crear el disquete de arranque, necesitarás descargar la imagen de disco
de la `página de descargas`_, extraer el archivo, y copiar la imagen a un disquete.
Si usas un systema operativo como UNIX (como Linux o FreeBSD), puedes hacerlo con el
siguiente comando::

    > cd AROS-<version>-i386-pc-boot-floppy
    > dd if=aros.bin of=/dev/fd0

Si usas Windows, necesitarás obtener rawrite_ to copiar la imagen al disquete. Mira
en la documentación de rawrite_ para aprender a usarlo. También hay una versión GUI
llamada rawwritewin.


El arranque
'''''''''''

Simplemente inserta el disquete de arranque en la unidad y reinicia la computadoa.
El arranque es completamente automático, y si todo va bien deberías ver una linda
pantalla después de un momento.

Instalando en el disco rígido
"""""""""""""""""""""""""""""

Bueno, fíjate que has sido **ADVERTIDO** que la instalación en el disco duro está
incompleta y es **peligrosa** para cualquier dato, así que asegúrate que la
PC que estés usando no tiene ningún dato útil. Se recomienda usar una máquina
virtual, ya que minimiza cualquier riesgo posible y permite que AROS sea usado
y probado en una máquina funcionando (aunque emulada). Ahora hay muchas VM
disponibles, como QEMU y VMWare.

Configurando el disco duro
^^^^^^^^^^^^^^^^^^^^^^^^^^

Primero, prepara tu disco duro -sea real o una unidad virtual-. En una unidad
real esto significa enchufarla a la máquina (siempre un buen comienzo) y 
configurar el BIOS. Para una unidad virtual de un emulador o de un virtualizador
probablemente necesitarás seleccionar una opción para crear una nueva imagen
de unidad, y establecerla como una de las unidades de arranque de la PC virtual
(la unidad de CD debe ser el primer dispositivo de arranque durante la
instalación de AROS).

Otro paso será limpiar el disco duro de cualquier partición existente, para
quitar cualquiera que pueda evitar la creación de nuestra partición.
Es posible instalar AROS con otro OS, pero se necesitarán más habilidades y aquí
no es tratado. Por el momento, aprenderemos cómo instalar AROS como el único
sistema en el disco rígido.

Particionado
^^^^^^^^^^^^

Instalación en la única partición

Aquí aprenderemos cómo instalar AROS como el único sistema en la PC y
ser puesto en la única partición. Este es el caso más fácil de instalación.

Este capítulo puede ser un poco engañoso, porque la característica de instalación
está incompleta. Primero, recuerda una regla común para este proceso -*reinicia*
después de cada cambio significativo hecho al sistema de archivo (indicaremos
dónde es necesario). Reiniciar significa cerrar la ventana HDToolbox si está 
abierta y reinicar la computadora o máquina virtual, así que se trata de un reinicio
por hardware (hard reset). También puedes probar un reinicio por software (soft
reset) tipeando <reboot> ENTER en la ventana del CLI.

Primero, encuentra una herramienta en el CD de AROS llamada *HDToolBox*. Está
en el cajón Tools. Ésta será tu atormentador del disco rígido por un tiempo.
Cuando la ejecutes, verás una ventana con un selector device-type (tipo de dispositivo).
En este ejemplo (de aquí en más), estamos usando una unidad de disco rígido real o virtual
IDE (también conocida como ATA). Así que apretando (hacer click) en la entrada
*ata.device* mostrará en la ventana izquierda Devices:1. Éste es nuestro disco duro.
Apretando de nuevo en esta entrada se mostrará la lista de discos duros disponibles.

Así que deberíamos ver a nuestro disco en la lista. Si es uno virtual, veremos
algo como *QEMU Harddisk* o el equivalente de VMWare. Si el disco es real,
deberías ver su nombre. Si esto no ocurre, deberías asegurarte que has preparado
correctamente tu disco duro. Apretando en el nombre del HD veremos cierta 
información::

    Size: <Tamaño del HD>
    Partition Table: <Tipo de la actual PT; debería ser unknown (desconocida) después del borrado>
    Partitions: <cantidad de particiones en el HD; debe ser 0 porque recién hemos empezado>

Bien, ahora debemos crear una nueva tabla de particiones. Aquí, para una
PC debemos crear un tipo de tabla *PC-MBR*. Para hacerlo, presiona en el
botón *Create Table* y selecciona de la lista *PC-MBR*. Aprieta OK.

Después debemos escribir los cambios al disco. Para hacerlo, aprieta en 
el nombre del HD y después en el botón *Save Changes*. Responde *Yes* en 
el cuadro de diálogo de confirmación. Cierra la ventana del HDToolBox y
reinicia el sistema con el Live CD.

Después de que el sistema arranque, ejecuta de nuevo HDToolbox. Ahora, 
después de ingresar en la entrada *ata.device* debemos ver la información
"Partition table: PC-MBR, Partitions:0". Así está bien, todavía no hemos
establecido particiones. Hagámoslo ahora. Aprieta en el nombre del HD para
ir a la lista de particiones. La lista está vacía. Aprieta en el botón *Create
Entry*, selecciona todo el espacio apretando en un espacio vacío no
seleccionado y aprieta *OK*. Ahora deberías ver en la lista una entrada
"Partition 0". Escógela apretando para obtener esta información::

    Size: <Partition size. Almost equal to HD size>
    Partition table: Unknown <Not created yet>
    Partition type: AROS RDB Partition table <That's OK>
    Active: No <Not active>
    Bootable: No <Not bootable>
    Automount: No <Will not mount on system startup>

Aquí puede haber alguna diferencia - hacer una partición en una tabla RDB o
una usual partición PC-MBR. RDB (Bloque de Disco Rígido) es la opción de 
compatibilidad y fue usada en el particionado de los HDD de la Amiga, y también
podemos usarla. Aunque, AROS soporta las particiones FFS creadas dentro de una
común tabla PC-MBR, igual que una usual partición PC como FAT/NTFS/etc. El segundo
camino se puede considerar algo más moderna y más compatible con algunos programas
AROS. Consideremos ambas.

*FFS en RDB*
Ahora, aprieta el botón *Create Table*, selecciona *RDB table* y aprieta OK.
Para guardar los cambios, sube *un nivel* apretando el botón *Parent*,
selecciona de nuevo el nombre del HD y aprieta el botón *Save Changes*. Responde
*Yes* dos veces en el cuadro de diálogo de confirmación. Sal del HDToolbox y
reinicia la máquina.

*FFS en MBR*
... a agregarse.

Después de arrancar, ejecuta HDToolbox (ya habrás adivinado eso). Ahora la
información para nuestra Partición 0 es la misma excepto que la tabla de partición
ahora es RDB (o no). Esta partición debe ser puesta en Activa. Para hacerlo, 
aprieta el botón *Switches*, selecciona la casilla de verificación *Active*
y aprieta *OK*. Ahora qué. Sí, guarda los cambios subiendo un nivel y apretando
el botón. Sal y reinicia.

¿Por qué estamos reiniciando tanto? Bueno, el HDToolbox y las bibliotecas del
sistema están todavía sin terminar y son bastante defectuosas, así que reiniciar
después de cada paso ayuda a reestablecerlas a su estado inicial.

Después del arranque, HDToolbox debería mostrarnos que la Partición 0 se ha
vuelto activa. Eso es bueno, ahora debemos crear nuestro disco para instalar
AROS en él. Desciende un nivel apretando en la entrada "Partition 0". Ahora qué.
Sí, aprieta el botón Add Entry y selecciona todo el espacio vacío. Ahora verás
allí una entrada "DH0", que es nuestro disco. Apretando en él muestra la siguiente
información::

    Size: <well...>
    Partition Table: Unknown (it's OK)
    Partition Type: Fast Filesystem Intl <OK>
    Active: No <OK>
    Bootable: No <we must switch it to Yes>
    Automount: No <we must switch it to Yes>

Ahora, sube *dos niveles* hasta el nombre del HD, aprieta en Save Changes,
confirma, sal y reinicia. Después del arranque (¿no es bastante tedioso?),
¿qué deberíamos hacer? Sí debemos poner los switches a la unidad DH0 en
el HDToolbox. Vayamos a la entrada DH0 y pongamos los switches con el 
botón relevante y las casillas de verificación: *Bootable: Yes* y
*Automount: Yes*. Guarda los cambios después de subir dos niveles de nuevo,
confirma y reinicia.

*¿Cuánto resta?* Bien, estamos más allá de la mitad del camino. Después de
arrancar y revisar todas las configuraciones para HD0, debemos ver su OK.
Así que podemos salir del HDToolbox sin que quede ninguna duda. Este es el
momento para algo de la magia del CLI.

Formateo
^^^^^^^^

Para usarla, debemos formatear nuestra unidad DH0 recientemente creada. En este
momento AROS tiene dos opciones de sistema de archivos - Fast FileSystem (FFS) y
Smart FileSystem (SFS). A FFS se la conoce por ser algo más estable y compatible
con la mayoría de los programas, SFS es más a prueba de fallas y más avanzada, 
pero aún tiene algunas cuestiones con algunos programas. Hoy en día debemos poner
FFS porque el cargador de arranque GRUB no soporta SFS (GRUB2 sí lo hará). También
fíjate que puedes tener problemas usando algún software adaptado con SFS (como
gcc). Entonces ahora abre la ventana del CLI (botón derecho sobre el menú de arriba
y selecciona Shell del primer menú del Wanderer). En el prompt, ingresa el 
comando Info (tipea ``info`` y aprieta Enter). Deberías ver nuestro DH0 en la 
lista como ``DH0: Not a valid DOS disk``. Ahora lo formatearemos con el comando::

    >format DRIVE=DH0: NAME=AROS FFS INTL
    About to format drive DH0:. This will destroy all data on the drive. Are 
    you sure ? (y/N)

Ingresa y, aprieta Enter y espera un segundo. Deberías ver la cadena
``Formatting... done``. Si tienes un error, revisa todos los parámetros de la
partición en el HDToolbox, ya que puedes haber perdido algo, y repite.

Si estás experimentando problemas con el formato (como los mensajes ERROR, 
en especial cuando uses particiones en RDB), lo que es improbable, entonces puedes
probar una vieja y buena herramienta de Amiga, FORMAT64::

    >extras/aminet/format64 DRIVE DH0: Name AROS FFS INTL


Ahora el comando Info debería mostrar::

    >DH0: <size>  <used> <free> <full 0%> <errors> <r/w state> <FFS> <AROS>

Listo. Es el momento de reiniciar para la pre-instalación.

.. Nota:: Si todo esto te parece tan tedioso que no puedas seguirlo, hay algún
          alivio si pretendes usar AROS sólo en una máquina virtual.
          Primero, puedes conseguir un paquete pre-instalado, como *WinAROS/WinAROS
          Lite* - este sistema ya está instalado, pero puede estar atrasado.
          Segundo, puedes buscar el *Installation Kit* en los `AROS Archives`_
          que contiene un HD virtual hecho y listo para instalar, así que puedes
          saltar el procedimiento anterior e instalar una fresca versión de AROS.

Copiando el sistema
^^^^^^^^^^^^^^^^^^^

Después de reiniciar, notarás que nuestro HD AROS está en el escritorio, y
está vacío. Ahora necesitamos llenarlo con archivos.

Después de haber desarrollado el soporte para Arrastrar y Soltar en AROS el
sistema entero puede ser fácilmente copiado desde el LiveCD solamente arrastrando
los archivos al cajón DH0:. Lo que resta es reemplazar el archivo dh0:boot/grub/menu.lst
con dh0:boot/grub/menu_dh.lst.DH0.

En AROS hay un instalador, tan incompleto como lo es el HDToolbox, pero 
también se puede usar. Al menos, podemos probarlo. Entonces, aquí está la
primera manera para instalar.

1. Ejecuta *InstallAROS* en el cajón Tools. Verás la pantalla de bienvenida
diciéndote lo mismo que yo dije - que estamos usando la versión alfa. 
Saquémosle el jugo ;) Hay un botón *Proceed*
para que aprietes. Después, verás la Licencia Pública AROS, y deberías aceptarla
para seguir. Ahora verás la ventana con las opciones de instalación (si dice
No, sólo *desmarca* la casilla relevante) ::

    Show Partitioning Options...    []
        <No. As we've done that already>
    Format Partitions               []
        <No. We have done that already>
    Choose Language Options         []
        <No. It's better to do that later>
    Install AROS Core System        [V]
        <Yes, we need it. We're here to do that>
    Install Extra Software [V] 
        <Yes. Uncheck only if you want a lite installation>
    Install Development Software    []
        <No. This is mostly a placeholder at a moment>
    Show Bootloader Options         [V]
        <Yes, bootloader will not be installed otherwise>

Déjame advertir que *Show Partitioning Options* puede no ser seleccionable y estar
grisáceo en el caso que el instalador sea incapaz de hallar alguna partición
adecuada.

    Destination Drive
    [default:DH0]
    
    DH0  <that's correct>
    
    Use 'Work' Partition                        [] 
        <uncheck it, we're installing all-on-one>
    Copy Extras and Developer Files to Work?    [] 
        <same as above>
    Work drive ...
        <skipped>
    
Después de desmarcarla, aprieta *Proceed*. Aparece la ventana con las opciones 
del cargador de arranque. Aquí solamente podemos marcar si GRUB, el *GRand 
Unified Bootloader*, tiene que ser instalado en el DH0 y en cuál dispositivo.
Aprieta *Proceed* de nuevo.

Ahora la ventana dice que estamos listos para instalar. Aprieta *Proceed* de
nuevo. ¿Te gusta este bonito botón? ;)

Después de eso, la barra de progreso de la copia aparecerá a medida que los
archivos sean copiados. Espera hasta que el proceso termine. Después, tendrás
la pantalla de finalización y la casilla de verificación *Reboot*. Déjala
marcada y aprieta *Proceed*. No, eso no es todo - espera el paso restante. 
Ahora nuestra máquina reiniciará con las mismas configuraciones de antes,
desde el LiveCD.

Instalando el cargador de arranque
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Ahora veremos nuestro disco AROS con todos los archivos dentro. ¿No
habíamos instalado el cargador de arranque en los pasos previos? ¿qué
nos está faltando?
Bueno, si usas una fresca nightly build entonces 'GRUB <http://en.wikipedia.org/wiki/GRUB>'__ 
ya debería estar instalado.

Para las versiones más viejas (anteriores a nov. del 2006)
hay un bug en GRUB, que evita que se instale correctamente en el primer intento.
Así que no podrás arrancar y obtendrás los mensajes como GRUB GRUB FRUB etc.
Por favor lee lo siguiente.
La reinstalación en el segundo intento por lo general ayuda a resolver esto.
Así, que ahora necesitamos InstallAROS de nuevo. Repite todos los pasos anteriores
desde el punto 1, pero desmarca cada casilla de verificación. Después del último
el botón *Proceed*, se reinstalará GRUB, y aparecerá una ventana pidiéndote que
confirmes esa última escritura. Responde que sí tantas veces como sea necesario.
Ahora, en la última página, desmarca la casilla Reboot, cierra el programa Install
y apaga la máquina.

Alternativamente, GRUB puede ser instalado desde el shell con este comando::

    c:install-i386-pc device ata.device unit 0 PN <pn> grub dh0:boot/grub kernel dh0:boot/aros-i386.gz

donde PN <pn> (o PARTITIONNUMBER <pn>) es el número de partición donde se instalará
el cargador de arranque GRUB.


Preparando el arranque
^^^^^^^^^^^^^^^^^^^^^^

We have just done our first installation alchemy course, and AROS should
be ready now. We must remove the Live CD from the CD drive (or disable
booting from CD in VM) and check it out. Hear the drum roll? ;)
Hemos hecho nuestro primer curso de la alquimia de instalación, y AROS 
debería estar listo. Debemos retirar el Live CD de la unidad de CD (o deshabilitar
el arranque desde CD en la máquina virtual) y comprobar. ¿Oyes el redoble
de tambores? ;)

If something goes wrong, there can be some answers...
Si algo va mal, puede haber algunas respuestas...

Troubleshooting
^^^^^^^^^^^^^^^

El proceso de instalación es uno sobre los que se pregunta con más frecuencia
en los foros, en su mayoría por los recién llegados. Puedes revisar el FAQ para
ver si hay una respuesta a tus preguntas. ¿Alguna adición...?

Instalando AROS junto con otros sistemas
""""""""""""""""""""""""""""""""""""""""

En los pasos previamente descriptos hemos instalado AROS como el *único* sistema
en el HD. Pero, ¿puede ser instalado para un arranque múltiple con otros sistemas
en el HD? Sí. Pero de nuevo, esta tarea será dificultosa. 

AROS y Windows

Consideremos la situación cuando tienes solamente Windows(tm) XP instalado y
quieres poner AROS con él.
Los sistemas Windows NT se pueden instalar en los sistema de archivo FAT y NTFS.
Mientras NTFS es la manera más segura y robusta, no está soportada por GRUB
(desafortunadamente).

AROS y Linux (y otro OS que use el cargador GRUB)

Consideremos la situación cuando quieres tener tres sistema en tu HD -
Windows, Linux y AROS.  

Preparando el HD
^^^^^^^^^^^^^^^^

Continuará...

AROS/i386-PPC-hosted
--------------------

Requirimientos
""""""""""""""

Falta que alguien lo escriba.

AROS/m68k-backport aka AfA
--------------------------

Este no es el sabor usual nativo/alojado de AROS, sino que es algo que se puede
llarmar *backport*. En realidad, es un conjunto de bibliotecas y binarios que
mejora las capacidades del original AmigaOS. AfA significa AROS para Amigas.

Requirimientos
""""""""""""""

Falta que alguien lo escriba.

Instalación
"""""""""""

Instalación:

+ copia el directorio AfA_OS_Libs a la partición de arranque de tu unidad
  Amiga, sys:
  Si no te gusta aquí puedes copiarlo en algún otro lugar y asignar AfA_OS:
  al directorio en donde esté AfA_OS_Libs.
  copia Libs:freetype2.library en tu directorio sys:libs.
+ copia C:AfA_OS_Loader en tu directorio sys:fonts.
+ copia Fonts: en tu directorio sys:fonts. Si quieres tener más fuentes,
  usa las Fonts de AROS o de MOS.
+ copia prefs: a tu directorio sys:prefs.

Para iniciarlo, en tiempo de arranque, inserta AfA_OS_Loader en tu
S:startup-sequence, un poco antes de IPrefs. Debe ser insertado después de
parchar las herramientas como MCP o picasso96/cgx, porque ellas parchan a su
vez las funciones AfA_OS.

Si lo inicias con el parámetro MOUSESTART (debe estar escrito con mayúsculas),
debes mantener apretado el botón izquierdo del ratón durante el tiempo de arranque
para cargar los módulos, en vez de evitarlos.

Para ver que todo funcione bien, inicia el programa "TextBench" que está en este
archivo. La velocidad de suavizado de TTF no es en este momento un ganador en el
renderizado de texto, no está optimizado; mira el código fuente aatext, pero espero
que sea lo suficientemente rápido para ser usable incluso en una 060/50MHz.


Notas al pie
============

.. [#] En verdad *es* posible instalar AROS/i386-pc en el disco rígido, pero
       el procedimiento está lejos de estar automatizado y ser amigable para el 
       usuario y las herramientas necesarias están todavía en desarrollo y podrían
       ser bastante defectuosas. Por lo tanto, oficialmente no recomendamos la 
       instalación en el disco rígido para los usuarios sin experiencia en este
       momento en que se escribió esta nota.


.. _`página de descargas`: ../../download

.. _rawrite: http://uranus.it.swin.edu.au/~jn/linux/rawwrite.htm

.. _`AROS Archives`: http://archives.aros-exec.org
