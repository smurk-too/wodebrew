USB LOADER MRC v13
------------------
USB Loader mrc es un USB Loader minimalista para Wii, basado
en el c�digo original de Waninkoko y adapt�ndolo a los nuevos
juegos que han ido saliendo para aumentar la compatibilidad.
A diferencia de otros USB Loaders, �ste tiene las opciones
suficientes para poder jugar a todos los juegos posibles,
prescindiendo de multitud de opciones de configuraci�n que no
hacen m�s que dificultar y ralentizar la puesta a punto del
programa.

Caracter�sticas:
* USB Loader (no soporta SD)
* Minimalista
* 192x64 banners
* 128x176 portadas
* Soporte para pantalla panor�mica (m�s juegos en pantalla)
* Trucos Ocarina
* Selecci�n DOL alternativo autom�tico
* Bot�n A: ejecutar juego
* Bot�n HOME: abrir opciones
* Botones 1/2: ordenar juegos
* Bot�n B: cambio de vista
* Soporta mando GC
* Soporte para Diario Wii


C�mo cargar im�genes
--------------------
* Puedes descargarlas aqu�:
  http://usuaris.tinet.cat/mark/usbloader_mrc.cgi
  o descargarlas autom�ticamente en el men� HOME de USB Loader mrc
* Col�calas en sd:/usbloader_mrc/
* Las im�genes acabadas en _banner se usan en el modo de visualizaci�n
  de banners, y las acabadas en _cover se usan en el otro modo de portadas
  (puedes cambiar de modo de vista con el bot�n B).
* Puedes crear t� mismo las im�genes, si pulsas HOME podr�s ver el
  c�digo identificativo (6 caracteres) del juego seleccionado, basta
  con seguir las siguientes normas:
   * Formato PNG
      * en RGB
      * sin paleta indexada
   * Tama�os:
      * 192x64 para banners
      * 128x176 para portadas
   * Coletillas:
      * _banner.png para banners
      * _cover.png para portadas
   * Nombre archivo:
      * 3 primeros caracteres para cualquier regi�n del juego
      * los 6 caracteres para una regi�n del juego
      * ejemplo:
         * RSP_banner.png: cargar� el banner para cualquier
           versi�n de Wii Sports
         * RSP01_cover.png: cargar� la portada solo para
           la versi�n PAL de Wii Sports


C�mo cargar pieles (skins)
--------------------------
Si quieres modificar el aspecto visual de USB Loader mrc, puedes copiar
los archivos .PNG que hay dentro del subdirectorio /data/ del c�digo
fuente a sd:/usbloader_mrc/.

Una vez copiados, los puedes modificar a tu gusto teniendo en cuenta que:
 * el texto de abajo siempre se ver� de color blanco
 * no puedes cambiar el tama�o de las im�genes
 * se usa el color magenta (255,0,255) como transparente


Forzado de video
----------------
Si est�s intentando jugar a un juego de otra regi�n, posiblemente tengas
que forzar el modo de video al de la regi�n original del juego.

Para ello puedes modificar la opci�n Modo de video en el men� HOME
de USB Loader mrc.


Trucos Ocarina
--------------
Puedes usar trucos Ocarina para tus juegos. Para ello crea el archivo
.GCT con el Ocarina Codemanager (o similar) y col�calo en el directorio
sd:/usbloader_mrc/ con el nombre XXXXXX.gct, donde XXXXXX es el c�digo
identificativo del juego.

No est�s limitado a un archivo de trucos Ocarina por juego, puedes incluir
m�s si quieres (aunque solo se cargar� uno a la vez). Para que USB Loader
mrc pueda leer m�s archivos tienes que ponerles el nombre XXXXXX_Y.gct
donde Y puede tomar valor de 2 a 9. Es decir, puedes tener hasta 9 archivos
diferentes por juego.

Una vez tengas los archivos de trucos colocados en la SD, puedes activar
la opci�n de Trucos Ocarina en el men� HOME de USB Loader mrc.


Sistema de DOL alternativo
--------------------------
Algunos juegos requieren la ejecuci�n de otro archivo ejecutable para que
el juego funcione correctamente, como en el caso de Metroid Prime Trilogy,
por ejemplo.

USB Loader mrc tiene una base de datos interna con casi todos los juegos
que usan este sistema. Si el juego est� en ella no har� falta m�s que
activar la opci�n DOL Alternativo en el men� HOME de USB Loader mrc y
autom�ticamente te dejar� escoger entre los diferentes ejecutables .DOL
que tiene el juego. Normalmente solo hay uno, pero juegos como Metroid
Prime Trilogy o The House Of The Dead 2&3 RETURNS tienen varios.

Si el juego no est� en la base de datos interna, tendr�s que extraer el
archivo .DOL de la ISO de tu juego (con WiiScrubber, por ejemplo) y
colocarlo en el directorio sd:/usbloader_mrc/ con el nombre XXXXXX.dol,
donde XXXXXX es el c�digo identificativo del juego.

Para activar la opci�n de DOL alternativo (ya sea por SD o por base de
datos) debes entrar al men� HOME de USB Loader mrc.


Registro en el diario de Wii
----------------------------
Para que el tiempo de uso de los juegos se guarde en el diario de Wii como
si estuvieras jugando al juego con su disco, la opci�n Registrar en diario
de Wii viene activada por defecto.

Si no quieres que se guarde, puedes desactivar dicha opci�n dentro del
men� HOME de USB Loader mrc.


Historial
---------
v1 (2009.06.11)
* primera versi�n

v2 (2009.07.15)
* a�adida selecci�n de modo de video
* a�adida opci�n anti-error 002: juegos como Ghostbusters o Indiana Jones
  funcionan correctamente
* a�adida opci�n para ordenar juegos con bot�n 1
* ahora solo lee los banners de la p�gina a mostrar, as� carga m�s r�pido
  y si tienes los juegos favoritos en la primera p�gina te ahorras tiempo
* activada la opci�n de instalar o desinstalar juegos de nuevo
* soporte para mando de Gamecube
* Corregidos algunos bugs y 'warnings' del c�digo

v3 (2009.08.08)
* a�adido trucos ocarina
* a�adida carga de DOL alternativo
* ahora guarda configuraci�n de cada juego por separado

v4 (2009.09.05)
* a�adida selecci�n autom�tica de DOL alternativo desde base de datos
  interna
* ligera mejora de velocidad en carga de banners desde SD
* nuevo modo de vista con banners grandes (se mantienen los peque�os
  tambi�n)

* muchas correcciones de c�digo

v5 (2009.09.17)
* ya se pueden ordenar los juegos de dos formas (banners grandes y banners
  peque�os)
* soporte para hasta 3 archivos de trucos Ocarina por cada juego
* soporte para hasta 65000 como cifra del n�mero de archivo de DOL
  alternativo
* arregla un error introducido en la versi�n anterior en la que no iba el
  modo progresivo
* un poco de limpieza de c�digo
* m�s cosillas que no recuerdo

v6 (2009.10.13)
* carga much�simo m�s r�pido
* se incluyen dos versiones: la de siempre y una nueva con un modo de vista
  m�s que muestra portadas de 128x180 p�xeles

v7 (2009.11.17)
* a�adido parche de NEW Super Mario Bros. Wii en tiempo real (aunque
  con cIOSrev15 ya no es necesario)

v8 (2010.01.03)
* finalmente he decidido dar de baja los banners peque�os e incorporar
  las portadas oficialmente porque...
* ahora se pueden poner huecos en las p�ginas, as� el orden de los juegos
  por categor�as/p�ginas queda mucho mejor
* las portadas ahora son menos altas, de 128x176
* los banners grandes ahora usan la coletilla _banner y no _big.
* a�adido sonido de banner al pulsar A y oscurecimiento de la pantalla a
  modo de confirmaci�n
* otras mejoras de velocidad

v9 (2010.02.04)
* a�adido soporte para registrar tiempo de juego en Diario de Wii

v10 (2010.02.15)
* corregido el tiempo de registro en Diario de Wii, ahora empieza a
  contar desde que se ejecuta el juego y no desde que abrimos el lanzador
* a�adida opci�n individual para desactivar el registro de Diario de Wii
* a�adido soporte para pantalla panor�mica (m�s juegos en pantalla)
* ahora soporta hasta 9 archivos de trucos Ocarina
* a�adido soporte para pieles (skins) personalizadas
* corregidos code dumps con SDHC o SD incompatibles (te�ricamente, no he
  podido probarlo)
* varias mejoras de c�digo

v11 (2010.02.17)
* corregido un bug que clonaba juegos

v12 (2010.02.19)
* descarga autom�tica de portadas por internet (en el men� HOME)
* sistema de portadas universales: primero busca la portada XXXXXX_Y.png
  como antes, si no lo encuentra busca XXX_Y.png, donde X son los 3 primeros
  caracteres del identificador del juego
* opci�n para escoger a donde volver: HBC o men� de Wii (para usuarios de
  forwarders)
* corregida la opci�n de diario, ahora ya se guarda en la SD si escogemos que
  un juego NO se registre en el diario
* crea el directorio usbloader_mrc si no existe

v13 (2010.02.28)
* soporte para cIOSx rev18
* se puede desactivar el sonido del banner en las opciones
* nuevo formato de guardado de orden de juegos en prueba (mantengo compatibilidad
  con el antiguo para no tener que reordenar los juegos otra vez, pero la
  configuraci�n general como el tipo de pantalla o el modo de salida se tiene
  que volver a configurar)



--by Marc
http://usuaris.tinet.cat/mark/