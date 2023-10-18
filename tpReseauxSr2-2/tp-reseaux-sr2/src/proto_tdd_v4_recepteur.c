#include <stdio.h>
#include <stdlib.h>
#include "application.h"
#include "couche_transport.h"
#include "services_reseau.h"

/* =============================== */
/* Programme principal - récepteur */
/* =============================== */
int main(int argc, char* argv[]) {
    unsigned char message[MAX_INFO]; /* message pour l'application */
    paquet_t paquet, p_controle;        /* paquet utilisé par le protocole */
    int fin = 0, seq_attendu = 0;
    int service = 0, connect = 0;            /* condition d'arrêt */
    init_reseau(RECEPTION);

    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");


/* tant que le récepteur reçoit des données */
    while (!fin) {
      de_reseau(&paquet);
      if (verifier_controle(paquet)){
        switch(paquet.type){
    			case DATA:
            //attendre()
    				if (seq_attendu == paquet.num_seq) {
    					/* construction paquet */
              // extraction des donnees du paquet recu
    					for (int i=0; i<paquet.lg_info; i++) {
    						message[i] = paquet.info[i];
    					}
              // remise des donnees a la couche application
    					fin = vers_application_mode_c(T_DATA, message, paquet.lg_info);
    					p_controle.num_seq = paquet.num_seq;
              p_controle.lg_info = 0;
              p_controle.type = ACK;
              p_controle.somme_ctrl = generer_controle(p_controle);
    					inc(&seq_attendu);
              vers_reseau(&p_controle);
    				} /*else if (seq_attendu != paquet.num_seq){
    					vers_reseau(&p_controle);
    				}*/
    				break;

    			case CON_REQ:
    				if(connect){
              // si on est deja connecte on envoie l'ack
    					p_controle.type = CON_ACCEPT;
    					vers_reseau(&p_controle);
    				} else if(vers_application_mode_c(T_CONNECT, message, paquet.lg_info) == T_CONNECT_ACCEPT){
              // si la demande est accepte on envoie l'ack d'accepte
    					p_controle.type = CON_ACCEPT;
    					vers_reseau(&p_controle);
    					connect = 1;
    				} else {
              // sinon on refuse et quitte
    					p_controle.type = CON_REFUSE;
    					vers_reseau(&p_controle);
    					fin=1;
    				}
    				break;

    			case CON_CLOSE:
    				service = T_DISCONNECT;
    				vers_application_mode_c(service, message, paquet.lg_info);
    				p_controle.type = CON_CLOSE_ACK;
    				vers_reseau(&p_controle);
    				fin = 1;
    				break;
    		}
      }
    }

    printf("[TRP] Fin execution protocole transport.\n");
    return 0;

}
