import * as firebase from 'firebase/app'
import 'firebase/auth'
import 'firebase/database'

import * as videoplayer from './videoplayer';
import config from './config';

// Initialize Firebase
firebase.initializeApp(config.firebase);

var SimplePeer = require('simple-peer');
var wrtc = require('wrtc');

var remotePeer : any; // TODO: make it an array for one to many connection

var uniqid = require('uniqid');
const myPeerId = uniqid('peer-');
process.title = "Connecting as " + myPeerId;

export var roomLink: string = "not_set";
export function createRoom() {

    const roomId = uniqid('room-');

    remotePeer = new SimplePeer(
        { initiator: true, 
            trickle:false ,  
            wrtc: wrtc,
            config: { 
                iceServers: config.iceServers
            }
        });

    roomLink = "syncwatch://" + roomId;
 
    console.log("Creating room");
    console.log("Join link: " + roomLink);

    //videoplayer.setRoomInfo(roomLink);
    
    remotePeer.on('signal', (data: string) => {

        //console.log('SIGNAL', JSON.stringify(data));
    
        // add yourself to the room with your signaling data
        firebase.database().ref(roomId + "/" + myPeerId).push(data, 
            
            function(error: any){
                if (error) {
                    console.error(error)
                    return
                }
                console.log('Push successful')
                setNewPeer(myPeerId + " (you)");
            }
        );

        // and attempt to read others' signaling data
        readPeerSignals(roomId);
    });
    
    remotePeer.on('connect', () => {

        remotePeer.send(JSON.stringify({'info': 'Connection established'}));

        if (videoplayer.getCurrentSource() !== "")
            remotePeer.send(JSON.stringify({ "sourceURL": videoplayer.getCurrentSource()}));
        
        // wait for 'connect' event before using the data channel
        console.log("Connected to peer");
        //remotePeer.send('hey, how is it going?');
    });

    remotePeer.on('data', (data: string) => {
        // got a data channel message
        console.log('got a message: ' + data);
        handleReceived(data);
    });

    return roomLink;
}

export function join(roomId: string) {

    remotePeer = new SimplePeer({ wrtc: wrtc , trickle: false});

    console.log("Joining room: " + roomId);
    roomLink = "syncwatch://" + roomId;
    console.log("Reading signal data of peers in the room");
    
    // and add yourself to the room with your signaling data
    remotePeer.on('signal', (data: string) => {
 
        //peer2.signal(data);
        // adding it to the database acts as signaling assuming other peer 
        // will read it immediately and call signal on itself
        firebase.database().ref(roomId + "/" + myPeerId).push(data, 
            
            function(error: any){
                if (error) {
                    console.error(error)
                    return
                }
                console.log('Push successful')
                setNewPeer(myPeerId + " (you)");
            }
        );
    });

    remotePeer.on('data', (data: string) => {
        // got a data channel message
        console.log('got a message: ' + data);
        handleReceived(data);
    });

    // Read initiator's signal data
    readPeerSignals(roomId);
}

function readPeerSignals(roomId: string) {

    var roomRef = firebase.database().ref(roomId);
    roomRef.on("child_added", function(peer: any, prevChildKey: any) {
        
        
        console.log("Child added for " + peer.key);
        //console.log(peer.val());

        // for each peer in the room get signaling data unless the peer is yourself
        if (peer.key !== myPeerId) {

            //console.log(peer.val());
            // signaling data comes as json array
            peer.forEach(function(signalData: any) { 
                console.log("Adding signaling data for " + peer.key);
                //console.log(signalData.val() , "\n");
                remotePeer.signal(signalData.val()); 

                setNewPeer(peer.key);

            });  
        } else {
            console.log("Its own signal data, ignoring...");
        }

    });
}

exports.sendData = function(data: string) {

    console.log("Sending data: " + data);
    remotePeer.send(data);
}


function handleReceived(data: string) {

    var message = JSON.parse(data);
    var messageType = Object.keys(message)[0];

    switch (messageType) {

        case "videoState":

            // { "videoState": { "position": vid.currentTime, "paused": vid.paused } }
            videoplayer.setLastReceivedEvent(message);

            videoplayer.setPause(message["videoState"]["paused"]);
            videoplayer.setPosition(parseFloat(message["videoState"]["position"]));


            break;

        case "sourceURL":

            // { "sourceURL": url}
            videoplayer.start(message["sourceURL"]);
            
            break;

    }
}

var peersList = []
export var connectedDisplay = "None";
function setNewPeer(peerInfo: string | null) {

    if (peerInfo != null ) {

        if (connectedDisplay === "None")
            connectedDisplay = "";

        peersList.push(peerInfo);
        connectedDisplay += peerInfo + "\n";

    }
}