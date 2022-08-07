#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

using namespace std;

class Carretera
{

	int capacidad;
	bool direccionActual = false;//true: derecha, false: izquierda
	int cantidadActual = 0;
	int contadorInacionDerecha = 0;
	int contadorInacionIzquierda = 0;
	int limiteInacion;
	int esperandoIzquierda = 0;
	int esperandoDerecha = 0;
	bool bloqueoDerecha = false;
	bool bloqueoIzquierda = false;
	pthread_cond_t derecha = PTHREAD_COND_INITIALIZER;
	pthread_cond_t izquierda = PTHREAD_COND_INITIALIZER;
	pthread_mutex_t mvar = PTHREAD_MUTEX_INITIALIZER;

	public:
		Carretera(int c, int l);

		void PuntoEntrada(bool direccion){//true: derecha, false: izquierda
			pthread_mutex_lock(&mvar);

			//si no hay autos actualmente, la carretera toma la direccion del primero que entre
			if(cantidadActual == 0) direccionActual = direccion;


			if(direccion){
				esperandoDerecha++;//autos esperando a entrar por la derecha
				//si no hay mas capacidad, la direccion no corresponde o le toca el turno al otro lado, el auto espera
				while(cantidadActual >= capacidad || direccionActual != direccion || bloqueoDerecha){
					pthread_cond_wait(&derecha,&mvar);
					//si es el primero auto en iniciar la nueva direccion, cambia la direccion de la carretera e reinicia las variables de inacion
					if(cantidadActual == 0){
						direccionActual = direccion;
						contadorInacionIzquierda = 0;
						bloqueoIzquierda = false;
					}
				}
				esperandoDerecha--;
			}else{
				esperandoIzquierda++;//autos esperando a entrar por la izquierda
				//si no hay mas capacidad, la direccion no corresponde o le toca el turno al otro lado, el auto espera
				while(cantidadActual >= capacidad || direccionActual != direccion || bloqueoIzquierda){
					pthread_cond_wait(&izquierda,&mvar);
					//si es el primero auto en iniciar la nueva direccion, cambia la direccion de la carretera e reinicia las variables de inacion
					if(cantidadActual == 0){
						direccionActual = direccion;
						contadorInacionDerecha = 0;
						bloqueoDerecha = false;
					} 
				}
				esperandoIzquierda--;
			}

			cantidadActual++;//un auto ingresa a la carretera

			pthread_mutex_unlock(&mvar);
		}

		void PuntoSalida(){
			pthread_mutex_lock(&mvar);
			cantidadActual--;//un  auto sale de la carretera

			//si hay autos esperando para entrar por el otro lado, aumenta el contador de inacion correspondiente
			if(direccionActual && esperandoIzquierda > 0) contadorInacionDerecha++;
			if(!direccionActual && esperandoDerecha > 0) contadorInacionIzquierda++;

			//si el contador de inacion llega al limite, entonces no pueden entrar mas autos por ese lado para dejar espacio a los otros
			if(contadorInacionDerecha >= limiteInacion) bloqueoDerecha = true;
			if(contadorInacionIzquierda >= limiteInacion) bloqueoIzquierda = true;

			if(cantidadActual < 0) cantidadActual = 0;

			//si no hay autos en la carretera, y hay otros esperando en el otro lado, se les da el paso
			//si no, entonces se continua por el mismo lado
			if(direccionActual){
				if(cantidadActual == 0 && esperandoIzquierda > 0) pthread_cond_signal(&izquierda);
				else if(esperandoDerecha > 0) pthread_cond_signal(&derecha);
			}else{
				if(cantidadActual == 0 && esperandoDerecha > 0) pthread_cond_signal(&derecha);
				else if(esperandoIzquierda > 0) pthread_cond_signal(&izquierda);
			}

			pthread_mutex_unlock(&mvar);
		}
};

Carretera::Carretera(int c,int l){
	capacidad = c;
	limiteInacion = l;
}

Carretera monitor = Carretera(10,5);
int idAuto = 0;
pthread_mutex_t id = PTHREAD_MUTEX_INITIALIZER;


void *SimularAutos(void* arg){

	int cantidad = (int ) *((int *)arg + 1);
	int direccion = (int ) *((int *)arg);

	cout<<"cantidad: "<<cantidad<<endl;
	cout<<"direccion: "<<direccion<<endl;

	int autos[cantidad];
	for(int i = 0; i < cantidad;i++){
		pthread_mutex_lock(&id);
		autos[i] = idAuto;
		idAuto++;
		pthread_mutex_unlock(&id);
	}

	for(int i = 0; i < cantidad;i++){
		if(direccion) monitor.PuntoEntrada(true);
		else monitor.PuntoEntrada(false);
		cout<<"entra auto: "<<autos[i]<<" ";
		if(direccion) cout<<"por la derecha"<<endl;
		else cout<<"por la izquierda"<<endl;
		monitor.PuntoSalida();
		cout<<"sale auto: "<<autos[i]<<" ";
		if(direccion) cout<<"por la derecha"<<endl;
		else cout<<"por la izquierda"<<endl;
	}
}


int main(int argc, char const *argv[])
{
	pthread_t der1,der2;
    pthread_t izq1, izq2;

    int* arg1;
    int* arg2;
    int* arg3;
    int* arg4;

    arg1 = new int[2];
    arg2 = new int[2];
    arg3 = new int[2];
    arg4 = new int[2];

    *arg1 = 1;
    *(arg1 + 1) = 5;
    *arg2 = 1;
    *(arg2 + 1) = 5; 
    *arg3 = 0;
    *(arg3 + 1) = 5; 
    *arg4 = 0;
    *(arg4 + 1) = 5; 

    pthread_create(&der1,NULL,SimularAutos,arg1);
    pthread_create(&der2,NULL,SimularAutos,arg2);
    pthread_create(&izq1,NULL,SimularAutos,arg3);
    pthread_create(&izq2,NULL,SimularAutos,arg4);

    pthread_join(der1,NULL);
    pthread_join(der2,NULL);
    pthread_join(izq1,NULL);
    pthread_join(izq2,NULL);

    free(arg1);
    free(arg2);
    free(arg3);
    free(arg4);

	return 0;
}
