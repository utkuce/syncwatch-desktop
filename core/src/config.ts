var config : any = {

    firebase: {
        apiKey: "AIzaSyBkjYBi9s_72xopQA7bExRV9-5ULMkF5hg",
        authDomain: "syncwatch-e4b04.firebaseapp.com",
        databaseURL: "https://syncwatch-e4b04.firebaseio.com",
        projectId: "syncwatch-e4b04",
        storageBucket: "syncwatch-e4b04.appspot.com",
        messagingSenderId: "798396854377",
        appId: "1:798396854377:web:41db9be453d0e90d4bf2f8"
    },

    // https://global.xirsys.net/dashboard/services
    iceServers: [{
        urls: [ "stun:u3.xirsys.com" ]
     }, {
        username: "kegqbK2pge08X1_jWFkzLBfvuQiQLOLuKjytQRPMUrRXMzUhfctgzYZTIUyCxUKPAAAAAF4Xyy91dGt1Y2U=",
        credential: "b45e0378-3343-11ea-9c93-d2334316765f",
        urls: [
            "turn:u3.xirsys.com:80?transport=udp",
            "turn:u3.xirsys.com:3478?transport=udp",
            "turn:u3.xirsys.com:80?transport=tcp",
            "turn:u3.xirsys.com:3478?transport=tcp",
            "turns:u3.xirsys.com:443?transport=tcp",
            "turns:u3.xirsys.com:5349?transport=tcp"
        ]
     }]
}

export default config;