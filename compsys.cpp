/*
 * CompSys.cpp
 *
 *  Created on: Mar. 14, 2024
 *      Author: Hashim, Karyn, Omar
 *      mesage expected types: (we can add more for when it's coming from somewhere else)
 *      0x00 will be from radar
 *
 */

#include "CompSys.h"
#include "constants.h"
#include <iostream>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/dispatch.h>
using namespace std;

//*******************************************   COMPSYS SERVER SET UP  *******************************************
void* compsys_start_routine(void *arg) {
	CompSys &cs = *(CompSys*) arg;
	cs.server_start();

	return NULL;
}
//*******************************************   CONSTRUCTOR/ DECONSTRUCTOR  *******************************************
CompSys::CompSys() {

	int err_no;
	err_no = pthread_create(&compSys_thread_id, NULL, compsys_start_routine,
			this); //create the thread
	cout << "comp sys errno after thread create is " << err_no << endl;
	if (err_no != 0) {
		printf("COMPSYS: ERROR when creating the thread for comp sys \n");
	}

}
CompSys::~CompSys() {
	// TODO Auto-generated destructor stub
}

//*******************************************   COMPSYS SERVER FOR RADAR  *******************************************
void* CompSys::server_start() {
	cout << "COMPSYS: radar server start" << endl;
	name_attach_t *attach;
	plane_data msg;
	int rcvid;
	if ((attach = name_attach(NULL, ATTACH_POINT_COMP_SYS, 0)) == NULL) {
		perror("COMPSYS: Error occurred while creating the channel");
	}

	while (true) {
		// wait for a message
		rcvid = MsgReceive(attach->chid, &msg, sizeof(msg), NULL);
		if (rcvid == -1) {/* Error condition, exit */
			break;
		}

		if (rcvid == 0) {/* Pulse received */
			switch (msg.hdr.code) {
			case _PULSE_CODE_DISCONNECT:
				ConnectDetach(msg.hdr.scoid);
				break;
			case _PULSE_CODE_UNBLOCK:
				break;
			default:
				break;
			}
			continue;
		}

		/* name_open() sends a connect message, must EOK this */
		if (msg.hdr.type == _IO_CONNECT) {
			MsgReply(rcvid, EOK, NULL, 0);
			continue;
		}

		/* Some other QNX IO message was received; reject it */
		if (msg.hdr.type > _IO_BASE && msg.hdr.type <= _IO_MAX) {
			MsgError(rcvid, ENOSYS);
			continue;
		}

		// a radar message of plane info
		if (msg.hdr.type == 0x00) {
			// one plane recieved
			// print out the planes info for debugging purposes then send is out
			cout << "COMPSYS: DA plane! id: " << msg.id << " x: " << msg.px
					<< " y: " << msg.py << " z: " << msg.pz << " ax: " << msg.ax
					<< " ay: " << msg.ay << " az: " << msg.az << endl;

			addPlanetoList(msg.id, msg.px, msg.py, msg.pz, msg. ax, msg.ay, msg.az);
			// should add to some list where we can check if there are possible crashes
			// airscape.add(da plane)// should update the list if the plane is there already or add if not there
			// check_immediate crashes(); // woould go through the list to see if there are 2 planes in same spot
			// check_immediate crashes(); // woould go through the list to see if there will be a crash in one
		}
		// reply to radar
		MsgReply(rcvid, EOK, 0, 0);

	}

	/* Remove the name from the space */
	name_detach(attach, 0);

	return NULL;
}

//*******************************************   CLIENT TO COMMSYS  *******************************************
void* toCommSys(void *) {
	commSys_to_plane_cmd cmd;

	int server_coid; //server connection ID.

	if ((server_coid = name_open(ATTACH_POINT_COMP_SYS_COMM, 0)) == -1) {
		perror("Error occurred while attaching the channel");
	}


	while (true){
		cmd.hdr.type = 0x01;
		//for testing purposes
		cmd.cmd = "Test Command";
		cmd.id = 10102;

		if (MsgSend(server_coid, &cmd, sizeof(cmd), NULL, 0) == -1) {
			printf("Error while sending message to commsys");
			break;
		}
	}

	/* Close the connection */
	name_close(server_coid);
	return EXIT_SUCCESS;
}

// ------------------ Helpers for adding planes to list ------------------
// finds plane by ID
plane_data* CompSys::findPlaneById(int id){
	for(unsigned int i=0; i < planeList.size(); i++){
		if(planeList[i].id == id){
			printf("Plane id found: %d\n", planeList[i].id);
			return &planeList[i];
		}
	}
	printf("Plane not found\n");
	return nullptr;
}

// checks if plane is already in list
bool CompSys::isPlaneInList(int id){
	for(unsigned int i=0; i < planeList.size(); i++){
		if(planeList[i].id == id){
			return true;
		}
	}
	return false;
}

// add active plane to list
void CompSys::addPlanetoList(int newid, int newPx, int newPy, int newPz, int newAx, int newAy, int newAz){
	if(!isPlaneInList(newid)){
		plane_data newPlane;
		newPlane.id = newid;
		newPlane.px = newPx;
		newPlane.py = newPy;
		newPlane.pz = newPz;
		newPlane.ax = newAx;
		newPlane.ay = newAy;
		newPlane.az = newAy;
		planeList.push_back(newPlane);
	} else {
		cout << "Plane " << newid << " is already in the list." << endl;
	}
}

