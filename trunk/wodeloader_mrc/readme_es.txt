USB LOADER MRC v13
------------------
USB Loader mrc es un USB Loader minimalista para Wii, basado
en el código original de Waninkoko y adaptándolo a los nuevos
juegos que han ido saliendo para aumentar la compatibilidad.
A diferencia de otros USB Loaders, éste tiene las opciones
suficientes para poder jugar a todos los juegos posibles,
prescindiendo de multitud de opciones de configuración que no
hacen más que dificultar y ralentizar la puesta a punto del
programa.

Características:
* USB Loader (no soporta SD)
* Minimalista
* 192x64 banners
* 128x176 portadas
* Soporte para pantalla panorámica (más juegos en pantalla)
* Trucos Ocarina
* Selección DOL alternativo automático
* Botón A: ejecutar juego
* Botón HOME: abrir opciones
* Botones 1/2: ordenar juegos
* Botón B: cambio de vista
* Soporta mando GC
* Soporte para Diario Wii


Cómo cargar imágenes
--------------------
* Puedes descargarlas aquí:
  http://usuaris.tinet.cat/mark/usbloader_mrc.cgi
  o descargarlas automáticamente en el menú HOME de USB Loader mrc
* Colócalas en sd:/usbloader_mrc/
* Las imágenes acabadas en _banner se usan en el modo de visualización
  de banners, y las acabadas en _cover se usan en el otro modo de portadas
  (puedes cambiar de modo de vista con el botón B).
* Puedes crear tú mismo las imágenes, si pulsas HOME podrás ver el
  código identificativo (6 caracteres) del juego seleccionado, basta
  con seguir las siguientes normas:
   * Formato PNG
      * en RGB
      * sin paleta indexada
   * Tamaños:
      * 192x64 para banners
      * 128x176 para portadas
   * Coletillas:
      * _banner.png para banners
      * _cover.png para portadas
   * Nombre archivo:
      * 3 primeros caracteres para cualquier región del juego
      * los 6 caracteres para una región del juego
      * ejemplo:
         * RSP_banner.png: cargará el banner para cualquier
           versión de Wii Sports
         * RSP01_cover.png: cargará la portada solo para
           la versión PAL de Wii Sports


Cómo cargar pieles (skins)
--------------------------
Si quieres modificar el aspecto visual de USB Loader mrc, puedes copiar
los archivos .PNG que hay dentro del subdirectorio /data/ del código
fuente a sd:/usbloader_mrc/.

Una vez copiados, los puedes modificar a tu gusto teniendo en cuenta que:
 * el texto de abajo siempre se verá de color blanco
 * no puedes cambiar el tamaño de las imágenes
 * se usa el color magenta (255,0,255) como transparente


Forzado de video
----------------
Si estás intentando jugar a un juego de otra región, posiblemente tengas
que forzar el modo de video al de la región original del juego.

Para ello puedes modificar la opción Modo de video en el menú HOME
de USB Loader mrc.


Trucos Ocarina
--------------
Puedes usar trucos Ocarina para tus juegos. Para ello crea el archivo
.GCT con el Ocarina Codemanager (o similar) y colócalo en el directorio
sd:/usbloader_mrc/ con el nombre XXXXXX.gct, donde XXXXXX es el código
identificativo del juego.

No estás limitado a un archivo de trucos Ocarina por juego, puedes incluir
más si quieres (aunque solo se cargará uno a la vez). Para que USB Loader
mrc pueda leer más archivos tienes que ponerles el nombre XXXXXX_Y.gct
donde Y puede tomar valor de 2 a 9. Es decir, puedes tener hasta 9 archivos
diferentes por juego.

Una vez tengas los archivos de trucos colocados en la SD, puedes activar
la opción de Trucos Ocarina en el menú HOME de USB Loader mrc.


Sistema de DOL alternativo
--------------------------
Algunos juegos requieren la ejecución de otro archivo ejecutable para que
el juego funcione correctamente, como en el caso de Metroid Prime Trilogy,
por ejemplo.

USB Loader mrc tiene una base de datos interna con casi todos los juegos
que usan este sistema. Si el juego está en ella no hará falta más que
activar la opción DOL Alternativo en el menú HOME de USB Loader mrc y
automáticamente te dejará escoger entre los diferentes ejecutables .DOL
que tiene el juego. Normalmente solo hay uno, pero juegos como Metroid
Prime Trilogy o The House Of The Dead 2&3 RETURNS tienen varios.

Si el juego no está en la base de datos interna, tendrás que extraer el
archivo .DOL de la ISO de tu juego (con WiiScrubber, por ejemplo) y
colocarlo en el directorio sd:/usbloader_mrc/ con el nombre XXXXXX.dol,
donde XXXXXX es el código identificativo del juego.

Para activar la opción de DOL alternativo (ya sea por SD o por base de
datos) debes entrar al menú HOME de USB Loader mrc.


Registro en el diario de Wii
----------------------------
Para que el tiempo de uso de los juegos se guarde en el diario de Wii como
si estuvieras jugando al juego con su disco, la opción Registrar en diario
de Wii viene activada por defecto.

Si no quieres que se guarde, puedes desactivar dicha opción dentro del
menú HOME de USB Loader mrc.


Historial
---------
v1 (2009.06.11)
* primera versión

v2 (2009.07.15)
* añadida selección de modo de video
* añadida opción anti-error 002: juegos como Ghostbusters o Indiana Jones
  funcionan correctamente
* añadida opción para ordenar juegos con botón 1
* ahora solo lee los banners de la página a mostrar, así carga más rápido
  y si tienes los juegos favoritos en la primera página te ahorras tiempo
* activada la opción de instalar o desinstalar juegos de nuevo
* soporte para mando de Gamecube
* Corregidos algunos bugs y 'warnings' del código

v3 (2009.08.08)
* añadido trucos ocarina
* añadida carga de DOL alternativo
* ahora guarda configuración de cada juego por separado

v4 (2009.09.05)
* añadida selección automática de DOL alternativo desde base de datos
  interna
* ligera mejora de velocidad en carga de banners desde SD
* nuevo modo de vista con banners grandes (se mantienen los pequeños
  también)

* muchas correcciones de código

v5 (2009.09.17)
* ya se pueden ordenar los juegos de dos formas (banners grandes y banners
  pequeños)
* soporte para hasta 3 archivos de trucos Ocarina por cada juego
* soporte para hasta 65000 como cifra del número de archivo de DOL
  alternativo
* arregla un error introducido en la versión anterior en la que no iba el
  modo progresivo
* un poco de limpieza de código
* más cosillas que no recuerdo

v6 (2009.10.13)
* carga muchísimo más rápido
* se incluyen dos versiones: la de siempre y una nueva con un modo de vista
  más que muestra portadas de 128x180 píxeles

v7 (2009.11.17)
* añadido parche de NEW Super Mario Bros. Wii en tiempo real (aunque
  con cIOSrev15 ya no es necesario)

v8 (2010.01.03)
* finalmente he decidido dar de baja los banners pequeños e incorporar
  las portadas oficialmente porque...
* ahora se pueden poner huecos en las páginas, así el orden de los juegos
  por categorías/páginas queda mucho mejor
* las portadas ahora son menos altas, de 128x176
* los banners grandes ahora usan la coletilla _banner y no _big.
* añadido sonido de banner al pulsar A y oscurecimiento de la pantalla a
  modo de confirmación
* otras mejoras de velocidad

v9 (2010.02.04)
* añadido soporte para registrar tiempo de juego en Diario de Wii

v10 (2010.02.15)
* corregido el tiempo de registro en Diario de Wii, ahora empieza a
  contar desde que se ejecuta el juego y no desde que abrimos el lanzador
* añadida opción individual para desactivar el registro de Diario de Wii
* añadido soporte para pantalla panorámica (más juegos en pantalla)
* ahora soporta hasta 9 archivos de trucos Ocarina
* añadido soporte para pieles (skins) personalizadas
* corregidos code dumps con SDHC o SD incompatibles (teóricamente, no he
  podido probarlo)
* varias mejoras de código

v11 (2010.02.17)
* corregido un bug que clonaba juegos

v12 (2010.02.19)
* descarga automática de portadas por internet (en el menú HOME)
* sistema de portadas universales: primero busca la portada XXXXXX_Y.png
  como antes, si no lo encuentra busca XXX_Y.png, donde X son los 3 primeros
  caracteres del identificador del juego
* opción para escoger a donde volver: HBC o menú de Wii (para usuarios de
  forwarders)
* corregida la opción de diario, ahora ya se guarda en la SD si escogemos que
  un juego NO se registre en el diario
* crea el directorio usbloader_mrc si no existe

v13 (2010.02.28)
* soporte para cIOSx rev18
* se puede desactivar el sonido del banner en las opciones
* nuevo formato de guardado de orden de juegos en prueba (mantengo compatibilidad
  con el antiguo para no tener que reordenar los juegos otra vez, pero la
  configuración general como el tipo de pantalla o el modo de salida se tiene
  que volver a configurar)



--by Marc
http://usuaris.tinet.cat/mark/