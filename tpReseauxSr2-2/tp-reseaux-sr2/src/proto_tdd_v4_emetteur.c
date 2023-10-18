#include "application.h"
#include "couche_transport.h"
#include "services_reseau.h"
#include <stdio.h>
#include <stdlib.h>
#define NUM_SEQ_MAX 16

/* =============================== */
/* Programme principal - émetteur  */
/* =============================== */
int main(int argc, char *argv[]) {
  unsigned char message[MAX_INFO]; /* message de l'application */
  int taille_msg;                  /* taille du message */
  int taille_fenetre = 7;          // taille de la fenetre
  int borne_inf = 0;               // borne inférieure de la fenetre
  int curseur = 0;
  int code_retour, i, service = 0, connect = 1; // code de contrôle et compteur
  paquet_t p_controle, paquet[NUM_SEQ_MAX]; /* paquet utilisé par le protocole */

  init_reseau(EMISSION);

  printf("[TRP] Initialisation reseau : OK.\n");
  printf("[TRP] Debut execution protocole transport.\n");

  // lecture de donnees provenant de la couche application
  de_application_mode_c(&service, message, &taille_msg);

  if (service == T_CONNECT) {
    // si on recois une demande de connexion
    paquet[curseur].type = CON_REQ; // on initialise le type du paquet
    paquet[curseur].num_seq = 0;   
    paquet[curseur].lg_info = 0;
    paquet[curseur].somme_ctrl = generer_controle(paquet[curseur]);
    
    do {
      // on envoie cette demande au recepteur
      vers_reseau(&paquet[curseur]);
      depart_temporisateur(100);
      code_retour = attendre();
    } while (code_retour != PAQUET_RECU);
    //attendre();
    de_reseau(&paquet[curseur]);
    arret_temporisateur();

    switch (paquet[curseur].type) { // on regarde le type du paquet reçu
    case CON_ACCEPT:
      // on evoie la reponse positive a la couche application
      vers_application_mode_c(T_CONNECT_ACCEPT, message, taille_msg);
      break;

    case CON_REFUSE:
      // on evoie la reponse negatif(refuse) a la couche application et on
      // quitte
      vers_application_mode_c(T_CONNECT_REFUSE, message, taille_msg);
      exit(1);
      break;
    }
  }

  de_application_mode_c(&service, message, &taille_msg);

  /* tant que l'émetteur a des données à envoyer */
  while ((borne_inf != curseur) && service != T_DISCONNECT) {
    if (dans_fenetre(borne_inf, curseur, taille_fenetre) && taille_msg != 0) {
      /* construction paquet */
      for (i = 0; i < taille_msg; i++) {
        paquet[curseur].info[i] = message[i];
      }
      paquet[curseur].lg_info = taille_msg;
      paquet[curseur].type = DATA;
      paquet[curseur].num_seq = curseur;
      paquet[curseur].somme_ctrl = generer_controle(paquet[curseur]);

      // remise a la couche reseau
      vers_reseau(&paquet[curseur]);
      if (borne_inf == curseur) depart_temporisateur(100);
      inc(&curseur);
      de_application_mode_c(&service, message, &taille_msg);
    } else { //timeout
      code_retour = attendre();
      if (code_retour == PAQUET_RECU) {
        de_reseau(&p_controle);
        if (verifier_controle(p_controle) &&
            dans_fenetre(borne_inf, p_controle.num_seq, taille_fenetre)) {
          borne_inf = (p_controle.num_seq + 1) % NUM_SEQ_MAX;
          arret_temporisateur();
          if (borne_inf != curseur) depart_temporisateur(100);
        }
      } else {
        i = borne_inf;
        depart_temporisateur(100);
        while (i != curseur) {
          vers_reseau(&paquet[i]);
          inc(&i);
        }
      }
    }
  }

  // fin de transmission, on demande la connexion
  while(connect){ 
    paquet[curseur].type = CON_CLOSE; // on initialise le type du paquet
    paquet[curseur].num_seq = curseur;   
    paquet[curseur].lg_info = 0;
    paquet[curseur].somme_ctrl = generer_controle(paquet[curseur]);
  
    do {
      vers_reseau(&paquet[curseur]);
      depart_temporisateur(100);
      code_retour = attendre();
    } while (code_retour != PAQUET_RECU);
    arret_temporisateur();
    if (code_retour == PAQUET_RECU) {
      de_reseau(&p_controle);
      if(p_controle.type == CON_CLOSE_ACK) connect = 0;
    }
  }
  printf("[TRP] Fin execution protocole transfert de donnees (TDD).\n");
  return 0;
}
