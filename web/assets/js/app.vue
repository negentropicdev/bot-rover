<template>
    <div class="outer">
        <div class="container-app">
            <h2 class="center">Rover Remote</h2>
            <div v-if="incompatible">
                <p class="incompatible">This browser is incompatible with the Gamepad API.</p>
            </div><div v-else>
                <div v-if="!hasController">
                    <p class="center">Press a button on your gamepad/joystick to queue for driving.</p>
                </div>
                <div v-if="hasController">
                    <div class="container-control" v-if="active">
                        <img src="http://rover.dbommarito.com/cam/?action=stream" width="640" height="360">
                        <p class="has-control center" v-show="control">YOU HAVE CONTROL! Remaining: {{ remaining }} seconds.</p>
                        <p class="no-control center" v-show="!control">You do not have control yet.</p>
                    </div>
                    <div v-show="showJoin">
                        <label for="name">Pilot's Name:</label><input type="text" ref="name" placeholder="Pilot Name">
                        <button @click="startQueue">Queue</button>
                    </div>
                    <p class="center">Controller connected: {{ controller.id }}</p><br>
                    <p>Queued Pilots:</p>
                    <ul>
                        <li v-for="pilot in pilotQueue" :key="pilot">{{ pilot }}</li>
                    </ul>
                </div>
            </div>
        </div>
    </div>
</template>

<style scoped>
.outer {
    width: 100%;
}

.center {
    text-align: center;
}

.container-app {
    width: 640px;
    padding: 0;
    margin: 0;
    margin-left: auto;
    margin-right: auto;
}

.incompatible {
    background-color: red;
    color: white;
}

.container-control {
    width: 100%;
    padding: 0;
    margin: 0;
}

.has-control {
    background-color: green;
    color: white;
}

.no-control {
    background-color: yellow;
    color: black;
}
</style>

<script>
import axios from 'axios';

export default {
    name: 'App',
    data() {
        return {
            active: false,
            controller: null,
            incompatible: false,
            control: false,
            scanned: [],
            pilotQueue: [],
            queueInterval: null,
            queueing: false,
            updateInterval: null,
            stopTimeout: null,
            ws: null,
            stopTime: 0,
            now: 0,
        };
    },
    created() {
    },
    mounted() {
        if ('GamepadEvent' in window) {
            window.addEventListener("gamepadconnected",
                this.gamepadConnect.bind(this));
            
            this.queueInterval = setInterval(this.checkQueue.bind(this), 2000);
        } else {
            alert('Sorry but this browser does not fully support the Gamepad API, please try Firefox or Chrome.');
            this.incompatible = true;
        }
        console.log("Controller mounted");
    },
    computed: {
        hasController() {
            return this.controller != null;
        },
        showJoin() {
            return !this.active && !this.queueing;
        },
        remaining() {
            return Math.round((this.stopTime - this.now) / 1000);
        }
    },
    watch: {
        active(val) {
            if (val && this.updateInterval == null) {
                this.ws = new WebSocket("ws://rover.dbommarito.com/cmd");

                this.ws.addEventListener('open', this.wsOpen);
                this.ws.addEventListener('message', this.wsMessage);
                this.ws.addEventListener('close', this.wsClose);
                this.ws.addEventListener('error', this.wsError);
            }
        }
    },
    methods: {
        wsOpen(e) {
            this.updateInterval = setInterval(this.update.bind(this), 200);
            this.control = true;

            this.now = new Date().getTime();

            var timeout = 2 * 60 * 1000; //2 minutes
            this.stopTime = this.now + timeout;
            this.stopTimeout = setTimeout(this.disable.bind(this), timeout);
        },
        wsMessage(e) {
            console.log(e.data);
        },
        wsClose(e) {
            clearTimeout(this.updateInterval);
            this.control = false;
            this.ws = null;
        },
        wsError(e) {
            if (this.updateInterval != null) clearTimeout(this.updateInterval);
            this.control = false;

            if (this.ws) this.ws.close();

            this.ws = null;
            console.log(e);
        },
        gamepadConnect(e) {
            if (this.controller == null) {
                this.controller = navigator.getGamepads()[e.gamepad.index];
                console.log(this.controller.id);
            }
        },
        disable() {
            this.active = false;

            if (this.ws) this.ws.close(1000);

            this.stopTimeout = null;
            this.ws = null;

            axios.post("/pilot/done", {});
        },
        update() {
            if (!this.ws) {
                return;
            }

            var gp = navigator.getGamepads()[this.controller.index];

            this.now = new Date().getTime();

            var x = -gp.axes[0]; // flip x so that positive value rotates to left
            var y = -gp.axes[1]; // flip y so that positive value is forward
            
            this.ws.send(JSON.stringify({
                drive: y,
                turn: x,
                stop: false
            }));
        },
        startQueue() {
            if (this.$refs.name.value == '') {
                alert('You must enter a name to queue for piloting the rover.');
            } else {
                var t = this;
                t.queueing = true;
                axios.post('/pilot/queue', {
                    name: this.$refs.name.value
                }).then(function(res) {
                    t.active = res.data.active;
                    t.pilotQueue = res.data.queue;
                }).catch(function(err) {
                    alert('Problem joining queue: ' + err);
                });
            }
        },
        checkQueue() {
            var t = this;

            axios.post('/pilot/queue', {
                name: ''
            }).then(function(res) {
                t.active = res.data.active;
                t.pilotQueue = res.data.queue;

                if (t.active) {
                    clearInterval(t.queueInterval);
                    t.queueing = false;
                    t.queueInterval = null;
                }
            }).catch(function(err) {
                alert('Problem checking queue: ' + err);
            });
        }
    }
}
</script>