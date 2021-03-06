import * as firebase from 'firebase/app'
import 'firebase/auth'
import 'firebase/database'

import uniqid from 'uniqid'
import {isEqual} from 'lodash';

import * as videoplayer from '../desktop/videoplayer';
import * as tui from '../desktop/tui'

var firebaseConfig = {
    apiKey: "AIzaSyBXuvO9e7-ag2fqbSV5gPoBmOlCedfIHuk",
    authDomain: "syncwatch-25414.firebaseapp.com",
    databaseURL: "https://syncwatch-25414.firebaseio.com",
    projectId: "syncwatch-25414",
    storageBucket: "syncwatch-25414.appspot.com",
    messagingSenderId: "96966942702",
    appId: "1:96966942702:web:49af6d60d5668ffddcf8ff"
}

// Initialize Firebase
firebase.initializeApp(firebaseConfig);

var roomId: string;
export var roomLink: string = "not_set";

export function create() {

    roomId = uniqid('room-');
    roomLink = "syncwatch://" + roomId;
    
    var roomRef = firebase.database().ref(roomId);
    roomRef.set(null,

        function(error: Error | null){

            if (error) {
                console.error(error);
                return;
            }
            
            console.log('Room created on firebase database');
            console.log("Join link: " + roomLink);
            tui.setRoomLink(roomLink);
            join(roomId);
        }
    );
}

var lastSentEvent : any;
var lastReceivedEvent : any;

export function sendData(data: any) {

    if (isEqual(lastReceivedEvent, data))
        return;

    lastSentEvent = data;

    var roomRef = firebase.database().ref(roomId);
    roomRef.update(data,

        function(error: Error | null){

            if (error) {
                console.error(error);
                return;
            }
            
            console.log("Sent: " + JSON.stringify(data));
        }
    ); 
}

export function join(rId: string) {

    roomId = rId;
    tui.setRoomLink("syncwatch://" + roomId);

    const username = "Guest"; //TODO: modifiable

    const myUserId = uniqid('user-');
    var roomRef = firebase.database().ref(roomId);


    roomRef.child("users").update( {
            [myUserId]: "Guest"}, function(error: Error | null){

                if (error) {
                    console.error(error);
                    return;
                }
                
                console.log("Joined the room as " + myUserId + "(" + username + ")" );
                listenRoom(roomRef);
            }
    );

    roomRef.child("users/" + myUserId).onDisconnect().remove(
        function(error: Error | null) {
            if (error) {
                console.error(error);
                return;
            }
        }
    );
}

function listenRoom(roomRef: firebase.database.Reference) {

    roomRef.on("child_added", function(snapshot: any, prevChildKey?: string|null) {
        handleData(snapshot)
    });

    roomRef.on("child_changed", function(snapshot: any, prevChildKey?: string|null) {
        handleData(snapshot)
    });
}

function handleData(snapshot : any) {

    var eventType : string = snapshot.key;
    var data = snapshot.val();

    lastReceivedEvent = {[eventType]: data};
    if (isEqual(lastReceivedEvent, lastSentEvent))
        return;

    console.log("Received: " + JSON.stringify(lastReceivedEvent));

    switch (eventType) {

        case "videoState":

            // { "videoState": { "position": vid.currentTime, "paused": vid.paused } }
            videoplayer.setPause(data["paused"]);
            videoplayer.setPosition(parseFloat(data["position"]));

            break;

        case "sourceURL":

            // { "sourceURL": url}
            videoplayer.setSource(data, false);

            break;
        
        case "users":

            //{"users":{"user-0":"name1","user-1":"name2","user-3":"name3"}
            tui.setUsers(data)

    }
}