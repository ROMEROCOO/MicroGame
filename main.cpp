


//NOTA IMPORTANTE
//--------------------------------------------------------------------------------------------------------
//la primera parte esta en codigo orientado a objetos y la segunda parte en c normal mas abajo comentado PERO FUNCIONAL.
//---------------------------------------------------------------------------------------------------------



#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "Sprite.h"
#include "Spritetwo.h"
#include "Spritethree.h"

// Definiciones de pines
#define TFT_CS   6
#define TFT_DC   7
#define TFT_RST  10
#define TFT_MOSI 11
#define TFT_CLK  13
#define TFT_MISO 12

#define BOTON_DERECHA   18
#define BOTON_IZQUIERDA 17
#define BOTON_ARRIBA    20
#define BOTON_ABAJO     19
#define BUZZER_PIN 16

// Constantes del juego
const int SCREEN_WIDTH = 240;
const int SCREEN_HEIGHT = 320;
const int TILE_SIZE = 32;
const int GRID_ROWS = 16;
const int GRID_COLS = 15;
const int FRAME_DELAY = 150;

// Enumeración para tipos de bloques
enum TipoBloque { CESPED, TIERRA, TESORO, FUEGO, VACIO };

//se pone en privado los pines

//Composición es cuando una clase usa objetos de otras clases para implementar su funcionalidad.
//Composición |||	Herencia
//"Tiene un"|||	"Es un"

// Clase para manejar el audio del juego
class AudioManager { //declaracion de la clase
private:             //parte privada no accesible desde afuera
    int buzzerPin;   //atrubuto (pin del buzzer)

public:              //parte publica accesible desde afuera
    AudioManager(int pin) : buzzerPin(pin) {  //constructor que inicializa el pin del buzzer
        pinMode(buzzerPin, OUTPUT);
    }

    void sonidoTesoro() {            //metodo o funcion para el sonido del tesoro
        tone(buzzerPin, 1000, 200);
        delay(200);
        noTone(buzzerPin);
    }

    void sonidoFuego() {
        tone(buzzerPin, 300, 300);
        delay(300);
        noTone(buzzerPin);
    }

    void sonidoGameOver() {
        tone(buzzerPin, 600, 400);
        delay(400);
        tone(buzzerPin, 300, 600);
        delay(600);
        noTone(buzzerPin);
    }

    void efectoGameOver() {
        for (int freq = 800; freq >= 200; freq -= 50) {
            tone(buzzerPin, freq, 50);
            delay(60);
        }
        noTone(buzzerPin);
    }
};

// Clase para manejar los controles
class InputManager {
private:
    int botonArriba, botonAbajo, botonIzquierda, botonDerecha;

public:
    InputManager(int arriba, int abajo, int izquierda, int derecha) 
        : botonArriba(arriba), botonAbajo(abajo), botonIzquierda(izquierda), botonDerecha(derecha) {
        pinMode(botonArriba, INPUT);
        pinMode(botonAbajo, INPUT);
        pinMode(botonIzquierda, INPUT);
        pinMode(botonDerecha, INPUT);
    }

    bool isArribaPressed() { return digitalRead(botonArriba) == LOW; }
    bool isAbajoPressed() { return digitalRead(botonAbajo) == LOW; }
    bool isIzquierdaPressed() { return digitalRead(botonIzquierda) == LOW; }
    bool isDerechaPressed() { return digitalRead(botonDerecha) == LOW; }

    void printStatus() {
        Serial.print("Botones -> Arriba: ");
        Serial.print(digitalRead(botonArriba));
        Serial.print(", Abajo: ");
        Serial.print(digitalRead(botonAbajo));
        Serial.print(", Izquierda: ");
        Serial.print(digitalRead(botonIzquierda));
        Serial.print(", Derecha: ");
        Serial.println(digitalRead(botonDerecha));
    }
};

// Clase para el jugador
class GamePlayer {
private:
    int x, y;
    int frame;
    int lastX, lastY;
    Adafruit_ILI9341* screen;  // PUNTERO a la pantalla

public:
    GamePlayer(Adafruit_ILI9341* scr) : screen(scr) {
        x = SCREEN_WIDTH / 2 - 16;
        y = 16 + 4;
        frame = 0;
        lastX = x;
        lastY = y;
    }

    void update(InputManager& input) {
        input.printStatus();

        if (input.isArribaPressed()) y -= 6;
        if (input.isAbajoPressed()) y += 6;
        if (input.isIzquierdaPressed()) x -= 6;
        if (input.isDerechaPressed()) x += 6;

        x = constrain(x, 0, SCREEN_WIDTH - 32);
        y = constrain(y, 16, SCREEN_HEIGHT - 32);

        Serial.print("Posición del jugador -> X: ");
        Serial.print(x);
        Serial.print(", Y: ");
        Serial.println(y);
    }

    void draw() {
        if (lastX != x || lastY != y) {
            screen->fillRect(lastX, lastY, 32, 32, ILI9341_BLACK);
        }
        
        screen->drawRGBBitmap(x, y, Player[frame], 32, 32);
        
        lastX = x;
        lastY = y;
    }

    void updateFrame() {
        frame = (frame + 1) % 2;
    }

    int getX() const { return x; }
    int getY() const { return y; }
    int getCenterX() const { return x + 16; }
    int getCenterY() const { return y + 16; }
};

//---------------------------------------------------------------------romero
// Clase para fuegos móviles
class FuegoMovil {
public:
    int fila;
    int colInicial;
    int colActual;
    int direccion;

    FuegoMovil() : fila(0), colInicial(0), colActual(0), direccion(1) {}
    
    FuegoMovil(int f, int cI, int cA, int dir) 
        : fila(f), colInicial(cI), colActual(cA), direccion(dir) {}
};

// Clase para manejar la matriz del juego
class GameMatrix {
private:
    TipoBloque matriz[GRID_ROWS][GRID_COLS];
    Adafruit_ILI9341* screen;
    FuegoMovil fuegos[10]; //hasta 10 fuego
    int numFuegos;
    int fuegoFrame;

public:
    GameMatrix(Adafruit_ILI9341* scr) : screen(scr), numFuegos(0), fuegoFrame(0) {}

    void inicializar() {
        // Inicializar matriz
        for (int fila = 0; fila < GRID_ROWS; fila++) {
            for (int col = 0; col < GRID_COLS; col++) { //recorre toda la matriz
                if (fila == 0) {
                    matriz[fila][col] = CESPED; //fila cero siemrpe cesped 
                } else {
                    int r = random(100);
                    if (r < 5) {
                        matriz[fila][col] = TESORO;
                    } else if (r < 10) {
                        matriz[fila][col] = FUEGO;
                    } else {
                        matriz[fila][col] = TIERRA;
                    }
                }
            }
        }

        // Reubicación de cofres para evitar conflictos con fuegos
        for (int fila = 1; fila < GRID_ROWS - 1; fila++) {
            for (int col = 0; col < GRID_COLS; col++) {
                if (matriz[fila][col] == TESORO && matriz[fila][col + 1] == FUEGO) {
                    matriz[fila][col] = TIERRA;
                    matriz[fila - 1][col] = TESORO;
                }
            }
        }

        inicializarFuegos();
    }

    void inicializarFuegos() { //donde el bloque se llama fuego 
        numFuegos = 0;
        for (int fila = 0; fila < GRID_ROWS; fila++) {
            for (int col = 0; col < GRID_COLS; col++) {
                if (matriz[fila][col] == FUEGO && numFuegos < 10) {
                    fuegos[numFuegos] = FuegoMovil(fila, col, col, 1); //llama fuego movil
                    numFuegos++;
                }
            }
        }
    }

    void dibujarToda() {
        for (int fila = 0; fila < GRID_ROWS; fila++) {
            for (int col = 0; col < GRID_COLS; col++) {
                dibujarBloque(fila, col);
            }
        }
    }

    void dibujarBloque(int fila, int col) {
        int offsetY = 48;
        int x = col * TILE_SIZE;
        int y = offsetY + fila * TILE_SIZE;

        switch (matriz[fila][col]) {
            case CESPED: 
                screen->fillRect(x, y, TILE_SIZE, TILE_SIZE, ILI9341_GREEN); 
                break;
            case TIERRA: 
                screen->fillRect(x, y, TILE_SIZE, TILE_SIZE, 0xA145); 
                break;
            case TESORO: 
                screen->drawRGBBitmap(x, y, oro, 32, 32); 
                break;
            case FUEGO: 
                screen->drawRGBBitmap(x, y, fire[fuegoFrame], 32, 32); 
                break;
            case VACIO: 
                screen->fillRect(x, y, TILE_SIZE, TILE_SIZE, ILI9341_BLACK); 
                break;
        }
        screen->drawRect(x, y, TILE_SIZE, TILE_SIZE, ILI9341_BLACK);
    }

    TipoBloque verificarBloque(int centroX, int centroY) {
        int fila = (centroY - 48) / TILE_SIZE;
        int col = centroX / TILE_SIZE;

        if (fila >= 0 && fila < GRID_ROWS && col >= 0 && col < GRID_COLS) {
            TipoBloque bloque = matriz[fila][col];
            if (bloque == TESORO || bloque == FUEGO) {
                matriz[fila][col] = VACIO;
                dibujarBloque(fila, col);
                return bloque;
            }
        }
        return VACIO;
    }

    void actualizarFuegos() {  //movmeinto de fuegos
        fuegoFrame = (fuegoFrame + 1) % 2;

        for (int i = 0; i < numFuegos; i++) {
            int nuevaCol = fuegos[i].colActual + fuegos[i].direccion;

            if (nuevaCol < fuegos[i].colInicial - 2 || nuevaCol > fuegos[i].colInicial + 2) {
                fuegos[i].direccion *= -1;
            } else {
                if (matriz[fuegos[i].fila][fuegos[i].colActual] == FUEGO) {
                    matriz[fuegos[i].fila][fuegos[i].colActual] = TIERRA;
                }
                dibujarBloque(fuegos[i].fila, fuegos[i].colActual);

                if (matriz[fuegos[i].fila][nuevaCol] != TESORO) {
                    fuegos[i].colActual = nuevaCol;
                    matriz[fuegos[i].fila][fuegos[i].colActual] = FUEGO;
                    dibujarBloque(fuegos[i].fila, fuegos[i].colActual);
                }
            }
        }
    }
};

// Clase para manejar el HUD y estado del juego
class GameManager {
private:
    Adafruit_ILI9341* screen;
    int vidas;
    int score;
    bool gameOver;
    bool gameOverShown;

public:
    GameManager(Adafruit_ILI9341* scr) : screen(scr), vidas(3), score(0), gameOver(false), gameOverShown(false) {}

    void dibujarHUD() {
        screen->fillRect(0, 0, SCREEN_WIDTH, 16, ILI9341_NAVY);
        screen->setTextColor(ILI9341_WHITE);
        screen->setTextSize(1);
        screen->setCursor(5, 4);
        screen->print("Vidas: ");
        
        for (int i = 0; i < 3; i++) {
            if (i < vidas) 
                screen->fillRect(50 + i * 10, 5, 8, 8, ILI9341_RED);
            else 
                screen->drawRect(50 + i * 10, 5, 8, 8, ILI9341_WHITE);
        }
        
        screen->setCursor(120, 4);
        screen->print("Puntos: ");
        screen->print(score);
    }

    void procesarColision(TipoBloque bloque, AudioManager& audio) {
        if (bloque == TESORO) {
            score += 10;
            audio.sonidoTesoro();
        } else if (bloque == FUEGO) {
            vidas -= 1;
            audio.sonidoFuego();
            if (vidas <= 0) {
                gameOver = true;
                audio.sonidoGameOver();
            }
        }
    }

    void mostrarGameOver(AudioManager& audio) {
        if (!gameOverShown) {
            screen->fillScreen(ILI9341_BLACK);
            audio.efectoGameOver();

            screen->setTextColor(ILI9341_RED);
            screen->setTextSize(2);
            screen->setCursor(SCREEN_WIDTH/2 - 60, SCREEN_HEIGHT/2 - 20);
            screen->print("GAME OVER");
            screen->drawRGBBitmap(SCREEN_WIDTH/2 - 16, SCREEN_HEIGHT/2 + 10, Player[0], 32, 32);
            gameOverShown = true;
        }
    }

    bool isGameOver() const { return gameOver; }
    int getVidas() const { return vidas; }
    int getScore() const { return score; }
};

// Clase principal del juego
class Game {
private:
    Adafruit_ILI9341 screen;
    GamePlayer player;
    GameMatrix gameMatrix;
    GameManager gameManager;
    InputManager input;
    AudioManager audio;
    unsigned long lastFrameTime;

public:
    Game() : screen(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO),
             player(&screen),
             gameMatrix(&screen),
             gameManager(&screen),
             input(BOTON_ARRIBA, BOTON_ABAJO, BOTON_IZQUIERDA, BOTON_DERECHA),
             audio(BUZZER_PIN),
             lastFrameTime(0) {}

    void setup() {
        Serial.begin(9600);
        
        screen.begin();
        screen.setRotation(0);
        screen.fillScreen(ILI9341_BLACK);

        gameMatrix.inicializar();
        gameMatrix.dibujarToda();
        gameManager.dibujarHUD();
        player.draw();
    }

    void loop() {
        if (!gameManager.isGameOver()) {
            player.update(input);
            player.draw();

            // Verificar colisiones
            TipoBloque bloque = gameMatrix.verificarBloque(player.getCenterX(), player.getCenterY());
            if (bloque != VACIO) {
                gameManager.procesarColision(bloque, audio);
                gameManager.dibujarHUD();
            }

            if (millis() - lastFrameTime > FRAME_DELAY) {
                player.updateFrame();
                gameMatrix.actualizarFuegos();
                lastFrameTime = millis();
            }
        } else {
            gameManager.mostrarGameOver(audio);
        }
    }
};

// Instancia global del juego
Game juego;

void setup() {
    juego.setup();
}

void loop() {
    juego.loop();
}







/*


//---------------------------en c normal


#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "Sprite.h"
#include "Spritetwo.h"  // Cofre
#include "Spritethree.h" // Fuego animado

#define TFT_CS   6
#define TFT_DC   7
#define TFT_RST  10
#define TFT_MOSI 11
#define TFT_CLK  13
#define TFT_MISO 12

#define BOTON_DERECHA   18
#define BOTON_IZQUIERDA 17
#define BOTON_ARRIBA    20
#define BOTON_ABAJO     19
#define BUZZER_PIN 16

//como constantes de la matriz 
const int SCREEN_WIDTH = 240;
const int SCREEN_HEIGHT = 320;
const int TILE_SIZE = 32;
const int GRID_ROWS = 16;
const int GRID_COLS = 15;

int playerX = SCREEN_WIDTH / 2 - 16; //coloca el jugadfor en el centro
int playerY = 16 + 4; //pos del jugador vertical mas 4 posiciones para que no me interfiera con la matriz ajustando
int frame = 0; //animacion del jugador incia en 0 entre los dos sprites
int fuegoFrame = 0; //lo mismo para el fuego
int vidas = 3; //inicializamos las vidas
int score = 0;
bool gameOver = false;

// Inicializa la pantalla TFT con la biliteca adfruit...
Adafruit_ILI9341 screen(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO); //usa pines sppara comuncacion

unsigned long lastFrameTime = 0; // Variable para controlar el tiempo de animación
const int FRAME_DELAY = 150; // Tiempo entre frames de la animación

enum TipoBloque { CESPED, TIERRA, TESORO, FUEGO, VACIO }; //para definir los tipos de bloques
TipoBloque matriz[GRID_ROWS][GRID_COLS]; //matriz de bloques para ver donde va cada cosa

// Estructura para fuegos móviles
struct FuegoMovil {
    int fila; //iniciamos fila
    int colInicial; //colocamos la columna inicial donde aprece e fuego
    int colActual; //donde esta ahora
    int direccion; // 1: derecha, -1: izquierda
};

FuegoMovil fuegos[10]; // Máximo 10 fuegos móviles en un array
int numFuegos = 0; //inicializa en 0 fuegos

// Declaración de funciones
//si no agego esto cadaque agrego una funcion me da error jaja aprendi alas malas
void inicializarMatriz();
void inicializarFuegos();
void dibujarTodaLaMatriz();
void dibujarMatrizTerreno(int fila, int col);
void dibujarHUD();
void dibujarJugador();
void manejarInput();
void verificarBloqueActual();
void actualizarAnimacion();
void mostrarGameOver();
void sonidoTesoro();
void sonidoFuego();
void sonidoGameOver();

void setup() {
    Serial.begin(9600);
    pinMode(BOTON_ARRIBA, INPUT); //
    pinMode(BOTON_ABAJO, INPUT);
    pinMode(BOTON_IZQUIERDA, INPUT);
    pinMode(BOTON_DERECHA, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    screen.begin();
    screen.setRotation(0);
    screen.fillScreen(ILI9341_BLACK);

    inicializarMatriz();
    inicializarFuegos();
    dibujarTodaLaMatriz();
    dibujarHUD();
    dibujarJugador();
}


void loop() {
    if (!gameOver) { //si el juego no ha terminado hacer lo demas ajaj
        manejarInput(); // Permite que el jugador se mueva de inmediato (detecta las teclas)
        dibujarJugador(); // Redibuja el jugador sin esperar a los fuegos

        if (millis() - lastFrameTime > FRAME_DELAY) { //cada cuanto se actuliza todo en este caso 150ms (milis cuenta desde que inicio tood)
            actualizarAnimacion(); // Mueve los fuegos de forma independiente
            lastFrameTime = millis(); //espera un timepo
        }
    } else {
        mostrarGameOver(); //de loc ontrraio gameover
    }
}

void inicializarMatriz() {
    for (int fila = 0; fila < GRID_ROWS; fila++) { //recorre cada fila de la matriz
        for (int col = 0; col < GRID_COLS; col++) { //recorre cada columna de la matriz
            if (fila == 0) {
                matriz[fila][col] = CESPED; // la primera fila es el cesped  por que ==0
            } else {
                int r = random(100); // d elo contrario hacemos u ramdon de tesoro, fuego y tierra
                if (r < 5) { //random de 0 a 99
                    matriz[fila][col] = TESORO; //poca probabilidad  d etesoro
                } else if (r < 10) {
                    matriz[fila][col] = FUEGO; //un pooc mas para fuegos
                } else {
                    matriz[fila][col] = TIERRA; //lo demas para tierra
                }
            }
        }
    }

    // Ahora reubicamos cofres para lo de lso fuegos ajja
    for (int fila = 1; fila < GRID_ROWS - 1; fila++) { // Evitamos tocar bordes
        for (int col = 0; col < GRID_COLS; col++) {
            if (matriz[fila][col] == TESORO && matriz[fila][col + 1] == FUEGO) {
                matriz[fila][col] = TIERRA; // Limpiamos su celda anterior esto o use para que los fuegos no me lso borren
                matriz[fila - 1][col] = TESORO; // Movemos el cofre arriba
            }
        }
    }
}

// Inicialización de los fuegos móviles
// (Definición eliminada por duplicidad, ver definición más abajo)

// Dibujar toda la matriz
void dibujarTodaLaMatriz() {
    for (int fila = 0; fila < GRID_ROWS; fila++) {
        for (int col = 0; col < GRID_COLS; col++) {
            dibujarMatrizTerreno(fila, col); //llamamos la funcion de abajo
        }
    }
}

// Dibujar un solo bloque de la matriz
void dibujarMatrizTerreno(int fila, int col) { //tomamos los parametros antes iciicaliadps
    int offsetY = 48; // Desplazamiento vertical para la matriz para que no inicie arriab en cero
    int x = col * TILE_SIZE; //calculamos la posicion en x con estas coordenadad e baso patra mapear
    int y = offsetY + fila * TILE_SIZE; //calculamos la posicion en y

    switch (matriz[fila][col]) {
        case CESPED: screen.fillRect(x, y, TILE_SIZE, TILE_SIZE, ILI9341_GREEN); break; //en el caso de cesped piuntar verde
        case TIERRA: screen.fillRect(x, y, TILE_SIZE, TILE_SIZE, 0xA145); break; //en el caso de tierra pintamos un color marron
        case TESORO: screen.drawRGBBitmap(x, y, oro, 32, 32); break; //aca la cosa cambia en ves pintar llmamos a la imagen del cofre del srpite
        case FUEGO: screen.drawRGBBitmap(x, y, fire[fuegoFrame], 32, 32); break; //aca la cosa cambia en ves pintar llmamos a la imagen del fuego del srpite
        case VACIO: screen.fillRect(x, y, TILE_SIZE, TILE_SIZE, ILI9341_BLACK); break; //lode mas negro 
    }
    screen.drawRect(x, y, TILE_SIZE, TILE_SIZE, ILI9341_BLACK); //funcion de graficar
}

void dibujarHUD() {
    screen.fillRect(0, 0, SCREEN_WIDTH, 16, ILI9341_NAVY); //barra de arriba
    screen.setTextColor(ILI9341_WHITE);
    screen.setTextSize(1);
    screen.setCursor(5, 4);
    screen.print("Vidas: ");
    for (int i = 0; i < 3; i++) { //son 3 vidas  y verifica cuanrtas quedan y va reduciendo el color si se pierde alguna
        if (i < vidas) screen.fillRect(50 + i * 10, 5, 8, 8, ILI9341_RED); //si tien vida se pinta rojo
        else screen.drawRect(50 + i * 10, 5, 8, 8, ILI9341_WHITE); // si oierde vida se pinta blanco
    }
    screen.setCursor(120, 4); //posiciones simpemente
    screen.print("Puntos: ");
    screen.print(score);
}
void dibujarJugador() {
    static int lastX = playerX; //esto se usa para borrar su posicion anterior (sombras)
    static int lastY = playerY;//en ytambien
    
    // Borrar la posición anterior
    if (lastX != playerX || lastY != playerY) {
        screen.fillRect(lastX, lastY, 32, 32, ILI9341_BLACK); //dond eya paso pintar negro
    }
    
    // Dibujar el jugador en la nueva posición
    screen.drawRGBBitmap(playerX, playerY, Player[frame], 32, 32); //se dubuja con el sprite y ahora esos valores son los ultimos
    
    // Actualizar última posición registrada
    lastX = playerX; //aca como dije se actulizan aos ultimas posiciones
    lastY = playerY;

    // Imprimir en consola la nueva posición del jugador
    Serial.print("Posición del jugador -> X: ");
    Serial.print(playerX);
    Serial.print(", Y: ");
    Serial.println(playerY);
}

void manejarInput() {
    int oldX = playerX;
    int oldY = playerY;

    // Leer entradas de los botones e imprimir su estado en Serial
    Serial.print("Botones -> Arriba: ");
    Serial.print(digitalRead(BOTON_ARRIBA));
    Serial.print(", Abajo: ");
    Serial.print(digitalRead(BOTON_ABAJO));
    Serial.print(", Izquierda: ");
    Serial.print(digitalRead(BOTON_IZQUIERDA));
    Serial.print(", Derecha: ");
    Serial.println(digitalRead(BOTON_DERECHA));


    //velocudad del juador al final el numero
    if (digitalRead(BOTON_ARRIBA) == LOW) playerY -= 6; //si esta en low (presionado) moverse 6 pixeles todo el sprite del player
    if (digitalRead(BOTON_ABAJO) == LOW) playerY += 6;
    if (digitalRead(BOTON_IZQUIERDA) == LOW) playerX -= 6;
    if (digitalRead(BOTON_DERECHA) == LOW) playerX += 6;
     
    //restringe para que no se salga de la pantalla
    //constrain es una funcion que restringe el valor a un rango especifico
    playerX = constrain(playerX, 0, SCREEN_WIDTH - 32);
    playerY = constrain(playerY, 16, SCREEN_HEIGHT - 32);

    verificarBloqueActual(); //en cual bloque esta el player

    if (playerX != oldX || playerY != oldY) dibujarHUD(); //para que no se actualice el HUD si no se mueve el player
}

void verificarBloqueActual() {
    int centroX = playerX + 16; //marca el centro del player
    int centroY = playerY + 16;
    int fila = (centroY - 48) / TILE_SIZE; //pasamos acorrdendas de la fila y columna
    int col = centroX / TILE_SIZE; //obvio si arriba corrimos 48 aca se compensan para los calculos

    if (fila >= 0 && fila < GRID_ROWS && col >= 0 && col < GRID_COLS) {
        TipoBloque bloque = matriz[fila][col];

        if (bloque == TESORO) {
            score += 10;
            matriz[fila][col] = VACIO; //si tom el tesoor queda vacio
            dibujarMatrizTerreno(fila, col); // Redibuja solo el bloque tocado
            sonidoTesoro(); //lamaos esta fucnoin
        } else if (bloque == FUEGO) {
            vidas -= 1;
            matriz[fila][col] = VACIO;
            dibujarMatrizTerreno(fila, col); // Redibuja solo el bloque tocado
            sonidoFuego();
            if (vidas <= 0) {
                gameOver = true;
                sonidoGameOver();
            }
        }
    }
}

void dibujarBloque(int fila, int col) {
    int x = col * TILE_SIZE; //x desde le inicio
    int y = 48 + fila * TILE_SIZE; //y desde 48 la fila

    //como arriba definimos los tipo de bloques aca llamamos es afuncion y ya
    switch (matriz[fila][col]) {
        case CESPED:
            screen.fillRect(x, y, TILE_SIZE, TILE_SIZE, ILI9341_GREEN);
            break;
        case TIERRA:
            screen.fillRect(x, y, TILE_SIZE, TILE_SIZE, 0xA145);
            break;
        case TESORO:
            screen.drawRGBBitmap(x, y, oro, 32, 32);
            break;
        case FUEGO:
            screen.drawRGBBitmap(x, y, fire[fuegoFrame], 32, 32);
            break;
        case VACIO:  // Si el bloque ya fue recogido, lo borramos
            screen.fillRect(x, y, TILE_SIZE, TILE_SIZE, ILI9341_BLACK);
            return; // Evitamos que se dibuje otra cosa encima
    }
    
    screen.drawRect(x, y, TILE_SIZE, TILE_SIZE, ILI9341_BLACK);
}



// Inicialización de los fuegos móviles con su posición inicial y rango de movimiento
void inicializarFuegos() {
    numFuegos = 0; //contador en cero
    
    //si encunetra un fuego lo guarda en el array
    for (int fila = 0; fila < GRID_ROWS; fila++) {
        for (int col = 0; col < GRID_COLS; col++) {
            if (matriz[fila][col] == FUEGO && numFuegos < 10) {
                fuegos[numFuegos] = {fila, col, col, 1}; // Guarda posición original
                numFuegos++;                              //y 1 es hacia la derecha y 0 ala izquierda
            }
        }
    }
}

// Animación y movimiento de los fuegos dentro de su rango horizontal

void actualizarAnimacion() {
    frame = (frame + 1) % 2; //alternamos los sprites para que se muevan
    fuegoFrame = (fuegoFrame + 1) % 2;

    for (int i = 0; i < numFuegos; i++) {
        int nuevaCol = fuegos[i].colActual + fuegos[i].direccion;

        // Verificar si el fuego alcanzó su límite
        if (nuevaCol < fuegos[i].colInicial - 2 || nuevaCol > fuegos[i].colInicial + 2) {
            fuegos[i].direccion *= -1; // Cambia la dirección cuando llegue al límite
        } else {
            // **NO borrar cofres al mover fuego**
            if (matriz[fuegos[i].fila][fuegos[i].colActual] == FUEGO) {
                matriz[fuegos[i].fila][fuegos[i].colActual] = TIERRA; // Limpia celda anterior si no es un tesoro
            }
            dibujarMatrizTerreno(fuegos[i].fila, fuegos[i].colActual);

            // **Solo mover fuego si la nueva posición NO es un cofre**
            if (matriz[fuegos[i].fila][nuevaCol] != TESORO) {
                fuegos[i].colActual = nuevaCol;
                matriz[fuegos[i].fila][fuegos[i].colActual] = FUEGO;
                dibujarMatrizTerreno(fuegos[i].fila, fuegos[i].colActual);
            }
        }
    }
}
void mostrarGameOver() {
    static bool yaMostrado = false;
    if (!yaMostrado) {
        screen.fillScreen(ILI9341_BLACK);
        for (int freq = 800; freq >= 200; freq -= 50) {
            tone(BUZZER_PIN, freq, 50);
            delay(60);
        }
        noTone(BUZZER_PIN);

        screen.setTextColor(ILI9341_RED);
        screen.setTextSize(2);
        screen.setCursor(SCREEN_WIDTH/2 - 60, SCREEN_HEIGHT/2 - 20);
        screen.print("GAME OVER");
        screen.drawRGBBitmap(SCREEN_WIDTH/2 - 16, SCREEN_HEIGHT/2 + 10, Player[0], 32, 32);
        yaMostrado = true;
    }
}
// Sonido del tesoro
void sonidoTesoro() {
    tone(BUZZER_PIN, 1000, 200);
    delay(200);
    noTone(BUZZER_PIN);
}

// Sonido del fuego
void sonidoFuego() {
    tone(BUZZER_PIN, 300, 300);
    delay(300);
    noTone(BUZZER_PIN);
}

// Sonido del Game Over
void sonidoGameOver() {
    tone(BUZZER_PIN, 600, 400);
    delay(400);
    tone(BUZZER_PIN, 300, 600);
    delay(600);
    noTone(BUZZER_PIN);
}

*/