/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/

#include "project.h"
                                
#define tiempoAjuste min*100+decSeg*10+uniSeg                      // Funciones Definidas 
#define tiempoMemorizado memoriaTiempo[0]*100 + memoriaTiempo[1]*10 + memoriaTiempo[2]
#define segundos decSeg*10 + uniSeg
#define minutos min*100
            
uint16 miliseg = 0;                                                 // Variables para controlar el sistema                                             
uint8  estado = 0;                                                
uint8  configuracionCompleta = 0;
uint8  ajustar = 0;
uint16 conteo_350_ms;
uint16 seg5 = 0;
uint8  botonPresionado = 0;
uint8  ciclos = 0;
uint8  tipo = 0;

uint8  min = 0;                                                      // Variables para control de tiempo
uint8  decSeg = 0;                                                 
uint8  uniSeg = 0;   
uint8  Seg = 0;
uint8  Min = 0;
uint8  memoriaTiempo[3]={0,0,0};

void resetear(void);                                                // Funciones para la ejecucion de
void restablecerBrillo(void);                                       // las actividades implementadas
void titileo (uint8 posicion);                                     
void parpadear(void);
void analizar(void);
void temporizador(void);
void invertirSentido(void);

CY_ISR_PROTO(tarea);                                                // Declaracion de interrupciones
CY_ISR_PROTO(adj);
CY_ISR_PROTO(mseg);
CY_ISR_PROTO(Sentido);

int main(void)
{
    CyGlobalIntEnable;                                              // Inicializacion de interrupciones
    isr_mseg_StartEx(mseg);
    isr_adj_StartEx(adj);   
    isr_tarea_StartEx(tarea);
    isr_sentido_StartEx(Sentido);
                                                                    // Inicializacion de componentes
    Contador_ms_Start();
    Displays_Start();
    
    LCD_Start();
    LCD_ClearDisplay();
    
    Displays_Write7SegNumberDec(min*100+decSeg*10+uniSeg,1,3,Displays_ZERO_PAD);
    
    for(;;)
    {   
        if (estado == 0 && Dip_Switch_Tarea_Read()==0){            // - En este estado se Configuran
            titileo (1);                                                // los minutos
            if (ajustar !=0) analizar();                                // - Llama la funcion analizar si 
            if (seg5 == 4999){                                          // se presiona el boton de ajuste
                estado++;
                configuracionCompleta++;                                // Variable que cuenta la cantidad de 
                restablecerBrillo();                                    // Displays configurados
                seg5 = 0;
            }
        }
        if (estado == 1 && Dip_Switch_Tarea_Read()==0){            // En este estado se configuran 
            titileo (2);                                                // los segundos (decenas)
            if (ajustar !=0) analizar();
            if (seg5 == 4999){
                estado++;
                configuracionCompleta++;
                restablecerBrillo();                                    // Esta funcion permite restablecer el 
                seg5 = 0;                                               // brillo de cada display en caso de 
            }                                                           // que el sistema cambie su estado
        }
        if (estado == 2 && Dip_Switch_Tarea_Read()==0){            // En este estado se configuran
            titileo (3);                                                // los segundos (unidades)
            if (ajustar !=0) analizar();
            if (seg5 == 4999){
                estado=0;
                configuracionCompleta++;
                restablecerBrillo();
                seg5 = 0;
            }
        }
        
        if (Dip_Switch_Tarea_Read() == 1 && configuracionCompleta < 2) parpadear();    // Si la configuracion es incompleta, 
                                                                                       // se escriben ceros y parpadea el displa                                                                                                    
        if (Dip_Switch_Velocidad_Read()==0 && Dip_Switch_Tarea_Read()==1 && configuracionCompleta >= 2){
            if (miliseg==999){
                restablecerBrillo();
                temporizador();
            }
        }
        
        if (Dip_Switch_Velocidad_Read()==1 && Dip_Switch_Tarea_Read()==1 && configuracionCompleta >= 2){
            if (miliseg == 0 || miliseg == 250 || miliseg==500 || miliseg==750){
                restablecerBrillo();
                temporizador();
            }
        }
        
        if (seg5 == 4000 && ciclos == 1 && botonPresionado == 1 && BotonSentido_Read()== 1) invertirSentido();
    }
}


// Definicion de Funciones
void invertirSentido(void){                                                     // Condicion para invertir el sentido de conteo
    botonPresionado = 0;                                              
    seg5=0;
    if (tipo==0) {
        tipo=1; LCD_Position(2,5);LCD_PrintNumber(11);
    }
    else if (tipo==1){
     tipo=0; LCD_Position(2,5);LCD_PrintNumber(0); 
    }
}

void temporizador (void){
    if (Min + Seg == tiempoMemorizado) tipo = 0;
    
    if (tipo == 0)                           
    {
        if (Min == 0 && Seg == 0) tipo = 1;
        else{
            if (Seg==0){
                Min--;
                Seg=59;
            }
            else Seg--;
        }
        ciclos = 1;                                     // Luego de ejecutarse esta linea, indica que 
    }                                                   // ya ha comenzado el temporizador en forma descendente 
                                                        // y ya es posible invertir su sentido de conteo
    if (tipo == 1)                           
    {
        Seg++;
        if (Seg==60){
            Min++;
            Seg=0;
        }
                                          
    } 
    Displays_Write7SegNumberDec(Min,1,1,Displays_ZERO_PAD);
    Displays_Write7SegNumberDec(Seg,2,2,Displays_ZERO_PAD);
    
    LCD_Position(0,0); LCD_PrintString("minutos");LCD_Position(0,8);LCD_PrintNumber(Min);
    LCD_Position(1,0); LCD_PrintString("segundos");LCD_Position(1,8);LCD_PrintNumber(Seg);
    LCD_Position(3,0);LCD_PrintString("seg5");LCD_Position(3,8);LCD_PrintNumber(seg5); 
}

void analizar(void){
    if (conteo_350_ms <= 350 && ajustar==2){                                // Si se presiona 2 vece antes de 350ms el boton ajustar
        configuracionCompleta++;                                            // ejecutara esta accion
        estado++;
        restablecerBrillo();
        ajustar = 0;
        conteo_350_ms = 0;
        if (estado==3) estado = 0;
    }
    
    if (conteo_350_ms >= 351 && ajustar == 1){                              // De lo contrario, si pasados 350ms solo se presiono 1 vez
        ajustar=0;                                                          // ejecutara esta accion de ajuste de tiempo, la cual es funcion
        conteo_350_ms=0;                                                    // del estado en el cual se encuentre el sistema
        
        if (estado == 0) {
            min++;
            if (  min == 3 || ( min==2 && (decSeg > 2 || (decSeg > 1 && uniSeg > 2) ) )  ) min=0;
            memoriaTiempo[0] = min;
        }
        
        else if (estado == 1) {
            decSeg++;
            if ( decSeg == 6 || (min == 2 && ( (decSeg > 2)||(decSeg > 2 && uniSeg > 2) ) ) ) decSeg=0;
            memoriaTiempo[1] = decSeg;
        }
        
        else if (estado == 2) {
            uniSeg++;
            if ( uniSeg == 10 || (min == 2 && decSeg > 1 && uniSeg > 3 ) ) uniSeg = 0;
            memoriaTiempo[2] = uniSeg;
        }
        
        Displays_Write7SegNumberDec(tiempoAjuste,1,3,Displays_ZERO_PAD);
    }
}

void parpadear(void){                                                   // - Escribe ceros y parpadean a 1.5Hz (3 veces en 2 segundos)
    if (miliseg == 666){                              // si el sistema entra en modo 
        Dos_Puntos_Write(0);
        for (uint8 posicion_display = 0; posicion_display < 4; posicion_display++){
            Displays_SetBrightness(0,posicion_display);                 // visualizar y no se ha configurado el  
        }                                                               // tiempo por completo
    }                                                                   
    
    if (miliseg == 0){
        Dos_Puntos_Write(1);
        for (uint8 posicion_display = 0; posicion_display < 4; posicion_display++){
            Displays_Write7SegNumberDec(0,0,4,Displays_ZERO_PAD);
            Displays_SetBrightness(200,posicion_display);
            seg5 = 0;
        }
    }   
}

void titileo (uint8 posicion){                                                       // Recibe la posicion del display por 
    if (miliseg == 250 || miliseg == 750){                                           // configurar actualmente, y genera su 
        Displays_SetBrightness(0,posicion);                                          // titileo
        Displays_Write7SegNumberDec(tiempoAjuste,1,3,Displays_ZERO_PAD);
        Dos_Puntos_Write(0);
    }
    else if (miliseg == 0 || miliseg == 500) {
        Displays_SetBrightness(200,posicion);
        Dos_Puntos_Write(1);
    }
}

void restablecerBrillo(void){                                       // Pone alto el brillo de los displays
    Dos_Puntos_Write(1);
    for (uint8 posicion_display = 1; posicion_display < 4; posicion_display++)
        {
            Displays_SetBrightness(200,posicion_display);
        }
}

void resetear (void){
    Displays_ClearDisplayAll();
    Displays_Write7SegNumberDec(0,1,3,Displays_ZERO_PAD);
    restablecerBrillo();
    configuracionCompleta = 0;
    estado = 0;
    ajustar = 0;
    min = 0;
    seg5 = 0;
    decSeg = 0;
    uniSeg = 0;
}

// Definicion de Interrupciones
CY_ISR(mseg){                                                       // Se genera una interrupcion cada que el contador  
    conteo_350_ms++;                                                // cuente 1 mili segundo.
    miliseg++;                                                      // Ademas incrementa la variable 5seg que sirve de referencia
    seg5++;                                                         // para otras funciones 
    if(seg5 > 5000) seg5=0;
    if (miliseg == 1000) miliseg=0;
}

CY_ISR(adj){                                                        // Esta interrupcion permite ajustar el tiempo de conteo y reiniciar 
    seg5=0;                                                         // los 5 segundos que deben esperarse para cambiar de estado
    conteo_350_ms=0;                                                // cada que se es presionado el boton
    ajustar++;
}

CY_ISR(tarea){                                                      // Siempre que se mueva el DipSwitch se activara reset
    if (Dip_Switch_Tarea_Read() == 0) resetear();                   // para reiniciar el sistema en la funcion titileo dependiendo
    if (Dip_Switch_Tarea_Read() == 1){                              // del estado en el cual se encuentre el sistema
        Min=min;
        Seg=segundos;
        Dos_Puntos_Write(1);
    }
}

CY_ISR(Sentido){                                                    // Cada que se presione el boton, se hace seg5 = 0
    botonPresionado=1;                                              // si se deja undido, no cambio el valor de seg5
    seg5=0;                                                         // y comenzara a contar los 4 segundos condicionales
}

/* [] END OF FILE */