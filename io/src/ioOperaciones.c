// #include "ioOperaciones.h"
 
//  //Siempre que recibe una peticion se duerme 
//  int recibir_codigo_operacion(int socket) {
//      int cod_op;
//      if(recv(socket, &cod_op,sizeof(int), MSG_WAITALL) > 0 ){
//          return cod_op;
//      }else{
//          return -1;
//      }
 
// }

//  int manejar_peticiones(int socket, char* nombreIo, t_log* logger){
//      int cod_op;
//      while(1) {
//          cod_op = recibir_codigo_operacion(socket);
//          if(cod_op == PETICIONAIO){
//              int tiempo_sleep = recibir_peticion(socket);
//              usleep(tiempo_sleep);
//              avisar_despertar(socket, nombreIo);
//              continue;
//          }else if(cod_op == -1){
//              log_info(logger, "Hubo un error al recibir los datos");
//              return -1;
//          }else if(cod_op == 0) {
//              log_info(logger, "Se cerro el socket");
//              return -1;
//          }else {
//              log_info(logger, "NO corresponde a una operacion de IO");
//              continue;
//          }
         
//      }
 
//      return 0;
//  }
 
//  int recibir_peticion(int socket) {
 
//      int tiempoSuspension;
//      recv(socket, &tiempoSuspension, sizeof(int), MSG_WAITALL);
 
//      return tiempoSuspension;
//  }
 
 
//  // SE PODRIA HACER UNA FUNCION EN UTILS QUE MANDE DIRECTAMENTE UN TIPO DE DATO STRING 
//  void avisar_despertar(int socket, char* nombreIo){
//      t_paquete* paquete = malloc(sizeof(t_paquete));
 
//      paquete->codigo_operacion = DESPERTOIO;
//      paquete->buffer = malloc(sizeof(t_buffer));
//      paquete->buffer->size = strlen(nombreIo) + 1;
//      paquete->buffer->stream = malloc(paquete->buffer->size);
 
//      memcpy(paquete->buffer->stream, nombreIo, paquete->buffer->size);
 
//      int bytes = paquete->buffer->size + 2* sizeof(int);
//      void* a_enviar = serializar_paquete(paquete, bytes);
 
 
//      send(socket,a_enviar, bytes, 0);
 
//      free(a_enviar);
//      eliminar_paquete(paquete);
//  }