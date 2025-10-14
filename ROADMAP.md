# 🌱 Roadmap:

## 🎯 Objetivo inicial

Crear un juego simple donde el jugador cuida plantas en su balcón:

- La planta tiene niveles de agua, nutrientes y salud.
- El jugador puede regarla, nutrirla, podarla y eventualmente trasplantarla.
- Los valores cambian con el tiempo.

### Adicionales agregados sobre la marcha:

- Maceteros en un grid para tener varias plantas
- Tener distintos tipos de plantas
- Paso del tiempo: estaciones, etc
- Nivel de luz por planta y reacción a la luz
- Agregar a los niveles de la planta PH (por verse, pero deseable)
- Crecimiento de plantas (en un principio estirar el sprite o algo lmao)
  => Podar plantas

### Vision final (soñar es gratis)

Pseudo-simulador de cuidado de plantas que te enseña los cuidados de varias plantas al jugar

- Poner/quitar distintos tipos de maceteros para distintos tipos de plantas
- Distintos tipos de tierra (quizas hacer mezcla propia?)

Elementos sociales (ya pensando en un juego completo, pero esto puede ser parte de otro proyecto)

- Visitas de personas y posibilidad de invitarlas a tu jardin
- Segun dialogos y plantas que regales la relacion cambia y te pueden dar semillas/esquejes
- Minijuegos para obtener "materiales" que pueden ser
  => materiales: maceteros, etc
  => libros con informacion de ciertas plantas y sus cuidados

______________________________________________________________________

## 🗓️ Etapas del desarrollo

### ENGINE: refactorizaciones y mejoras al engine del juego

- [ ] Abstraccion de grid a su propia entidad y usarla para el garden,
      los botones, las posiciones de plantas para los maceteros, etc. 
- [ ] Uso de escenas (garden/main menu por ahora pero suficiente para prototipar)
  - [ ] Desacoplar logica de UI particular del garden y delegar al juego/escena
    i.e. (1) Juego tiene el estado del puntero, con eso UI dibuja el puntero en vez de necesitar
    saber que herramienta esta seleccionada (en otra escena puede que no haya herramientas).
    (2) La escena actual inicializa los elementos de la UI llamando a algo como:
    ui_clean, ui_addButtonPannel, ui_addTextBox, etc.
- [ ] Delegar el registrado de comandos en el keymap al juego/escena


### TODO: cosas ajenas a milestones particulares para hacer cuando me den ganas de hacerlo

- [ ] Registrar historial de acciones recientes (cola circular) 
  * quiero hacer una cola circular?

### ✅ Milestone: Núcleo del juego

- [X] Crear estructura `Planta` con `agua`, `nutrientes`, `salud`.
- [X] Implementar funciones para `regar()`, `nutrir()`, `actualizar()`.
- [X] Crear un ciclo de simulación por "tick" (ej: 1 por segundo).
- [X] Imprimir stats en consola o dibujar en ventana (con `raylib`).
- [X] Mover plant, uiButton y ui a sus propios archivos
- [X] Crear archivos de cabecera

### ✅ Milestone: Interacción del jugador

- [X] Agregar sistema de input: teclas para regar/nutrir/podar.
- [X] Dibujar planta en pantalla (circulo y maceta).
- [X] Implementar efectos negativos si no se cuida.
- [X] Lugares predeterminados para los maceteros. + agregar/quitar maceteros??

### ✅ Milestone: Mejoras antes de continuar

- [X] Sprite para macetero
- [X] Mostrar stats visuales (texto o barras). (listo en "Milestone: Núcleo del juego" pero hagamoslo mas boni)
- [X] No de-seleccionar planter cuando se clickea la UI
  => Esto lo hacemos procesando el click en la UI primero y "usando" el click para que en el garden no se procese

### Milestone: Seleccion de maceteros distintos con distintos tamaños (sin propiedades particulares por ahora)

- [X] Tipos de maceteros con distintas dimensiones (1x1, 1x3, 4x2, etc)
- [X] Seleccion de tipos de maceteros
- [X] Que tile hover muestre el espacio entero que usará el macetero
- [X] Sprite atlas para maceteros con vista de cada angulo y mostrar el sprite correspondiente segun rotacion
- [X] Que macetero de MxN acepter M*N plantas - para probar concepto
  - [X] mostrar N plantas en un macetero
  - [ ] que un macetero > 1x1 tenga tiles en donde se puede plantar, y en donde no

### Milestone: UI a la sims/RCT

- [X] Orden de dibujado de plantas y maceteros
- [-] Permitir mover maceteros;
  - [X] Mover macetero
  - [X] Mostrarlo con sus plantas en el cursor cuando se esta moviendo
  - [X] Traslucir las plantas en el lugar original del planter cuando se esta moviendo
- [X] Zoom y movimiento del garden (con limites para que no se salga de la pantalla)
  - [X] Zoom
  - [X] Movimiento
- [X] Giro del jardin (deseable x1000 pero pajero)
- [X] Giro de macetero con tecla

### ✅ Milestone: Paso del tiempo y reacción de plantas sobre el tiempo

Aquí me sobrecompliqué y en terminos de gameplay no agrega nada la complejidad que hay.
Evaluar un sistema más como minecraft, o simplemente más sencillo.

Pensando en que la planta cada cierto tiempo se actualiza, más que en cada frame se actualice "* deltatime"

- [ ] Implementar sistema de eventos temporales (simular paso del tiempo).
  - [X] Reloj
  - [ ] Plantas actuan segun ticks
  // La unica razon para hacer esto es para que plantas distintas se actualicen cada distintos ticks
  - [ ] Ticks de mundo != ticks de plantas | Por cada N ticks de mundo ocurren M ticks de planta segun la planta
- [ ] Distintos tipos de plantas
  - Planta puede que pueda estar más o menos tiempo sin agua o sin nutrientes
    => Podría hacerse que las plantas consuman menos de cada cosa dependiendo del tipo
    => Por cada tick de planta baja X candidad de nutrientes y agua
    => Dependiendo de la planta, estar sin agua por X cantidad de ticks puede bajar mucho o poco la salud

### ✅ Milestone: Evolución y ambiente - depende de [Paso del tiempo...](#milestone-paso-del-tiempo-y-reacción-de-plantas-sobre-el-tiempo)

- [ ] Añadir sistema de luz solar y temperatura.
  => Agregar zonas con mayor o menor luz (parte del struct grid probablemente)
- [ ] Agregar comportamiento diferente a distintas plantas sobre la luz/temperatura
  => +-salud por tick? segun temperatura y/o luz?
  => Luz y temperatura son diferentes atributos?
  => Luz a-la minecraft:
    * Si tile (0,0) tiene luz 5, tile (0,1) tiene luz 4, etc.
    * Nivel de luz conceptualizado: 5-4 = luz directa, 3-2 = luz indirecta, 1-0 = sombra
    * Luz tarde/mañana? son realmente diferentes? o es solo cosa de temperatura?
    * Onda tile (0,0) con luz directa 5, puede ser:
      . verano: luz AM que es temperatura 3, o PM que es temperatura 5
      . invierno: luz AM que es temperatura 3, o PM que es temperatura 1

### ✅ Milestone: Persistencia

- [ ] Agregar guardado/carga en archivo (`.txt`, `.json`, etc.).
  => UI de menu principal Cargar/Nuevo
  => Definir estructura de datos para determinar como guardarlos
- [ ] 3 slots para guardar
  => fecha de inicio / fecha de modificacion
  => fechas guardadas en el mismo savefile para simplificar

### ✅ Milestone: UI/UX

- [ ] Agregar retroalimentación visual/auditiva al interactuar.
- [ ] Agregar música de fondo.
- [ ] Agregar indicadores de estados (`saludable`, `marchita`, `muerta`, etc. segun indice de salud).
- [ ] Agregar zoom/movimiento de garden
- [ ] Agregar texturas para bordes y/o fondos de elementos de la UI

### ✅ Milestone: Configuraciones de usuario

- [ ] Permitir cambiar la resolución del juego
- [ ] Permitir cambiar los keybinds sobre las acciones
  => En su defecto, mostrar lista de keybinds
- [ ] Permitir cambiar de pantalla completa a ventana/ventana sin bordes
- [ ] Permitir cambiar brillo? ni idea como hacer esto xd

## 🔁 Iteraciones futuras (visión larga)

- [ ] Sistema de trasplante a nuevas macetas.
- [ ] Simulación más profunda: enfermedades, estaciones, genética.

## 📦 Stack

- **Lenguaje:** C
- **Librería gráfica:** raylib
- **IDE/editor:** Neovim (btw).
- **Compilador:** gcc
- **Sistema de build:** Makefile

## 🧠 Posibles lecturas o tópicos de estudio:

- Leer: *C Programming: A Modern Approach* – K. N. King
- Practicar: estructuras de datos como listas, colas, árboles simples
- Entender: cómo funciona el heap, stack, y manejo de memoria en C
- Explorar: raylib, ejemplos y herramientas como `raygui`, `raudio`, etc.
