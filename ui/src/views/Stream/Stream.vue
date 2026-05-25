<template>
    <div class="stream-page">
        <div class="stream-toolbar">
            <span class="toolbar-label">View</span>
            <div class="view-btn-group">
                <button
                    v-for="v in viewOptions"
                    :key="v.value"
                    :class="{ active: selectedView === v.value }"
                    @click="selectView(v.value)"
                >
                    {{ v.label }}
                </button>
            </div>
            <div class="connection-badge">
                <span :class="['dot', wsState]" />
                {{ wsStateLabel }}
            </div>
        </div>

        <div :class="['stream-panel', panelClass]">
            <div
                v-for="ch in channels"
                :key="ch.id"
                class="stream-viewport"
            >
                <div class="viewport-header">{{ ch.label }}</div>
                <div class="viewport-body">
                    <canvas
                        v-if="ch.hasSignal"
                        :ref="el => { if (el) canvasRefs[ch.id] = el }"
                        :width="ch.width"
                        :height="ch.height"
                    />
                    <div v-else class="no-signal">
                        <span class="no-signal-icon">&#9634;</span>
                        <span>No signal</span>
                    </div>
                </div>
            </div>
        </div>
    </div>
</template>

<script>
import { mapGetters } from 'vuex';

const VIEW_OPTIONS = [
    { value: 'cl_view',        label: 'CL',       channels: ['cl']       },
    { value: 'ir_view',        label: 'IR',       channels: ['ir']       },
    { value: 'both_views',     label: 'Both',     channels: ['cl', 'ir'] },
];

const CHANNEL_META = {
    cl: { id: 'cl', label: 'CL View',  width: 640, height: 480 },
    ir: { id: 'ir', label: 'IR View',  width: 640, height: 480 },
};

export default {
    name: 'StreamView',
    data() {
        return {
            viewOptions:   VIEW_OPTIONS,
            selectedView:  'cl_view',
            wsState:       'disconnected',
            ws:            null,
            canvasRefs:    {},
        };
    },
    computed: {
        ...mapGetters('authentication', ['csrfToken']),
        currentOption() {
            return VIEW_OPTIONS.find(v => v.value === this.selectedView) || VIEW_OPTIONS[0];
        },
        channels() {
            return this.currentOption.channels.map(id => ({
                ...CHANNEL_META[id],
                hasSignal: false,
            }));
        },
        panelClass() {
            return this.channels.length > 1 ? 'dual' : 'single';
        },
        wsStateLabel() {
            return { connected: 'Connected', connecting: 'Connecting…', disconnected: 'Disconnected' }[this.wsState];
        },
    },
    watch: {
        selectedView() {
            this.reconnect();
        },
    },
    mounted() {
        this.connect();
    },
    beforeUnmount() {
        this.closeWs();
    },
    methods: {
        selectView(view) {
            this.selectedView = view;
        },
        connect() {
            this.closeWs();
            this.wsState = 'connecting';

            const proto  = location.protocol === 'https:' ? 'wss' : 'ws';
            const url    = `${proto}://${location.host}/api/v1/stream?view=${this.selectedView}`;

            this.ws = new WebSocket(url);
            this.ws.binaryType = 'arraybuffer';

            this.ws.onopen    = () => { this.wsState = 'connected'; };
            this.ws.onclose   = () => { this.wsState = 'disconnected'; };
            this.ws.onerror   = () => { this.wsState = 'disconnected'; };
            this.ws.onmessage = (evt) => this.handleFrame(evt.data);
        },
        reconnect() {
            this.connect();
        },
        closeWs() {
            if (this.ws) {
                this.ws.onclose = null;
                this.ws.close();
                this.ws = null;
            }
            this.wsState = 'disconnected';
        },
        handleFrame(data) {
            // Protocol: [1-byte channel tag (0x00=CL, 0x01=IR)][JPEG bytes...]
            const bytes     = new Uint8Array(data);
            const channelId = bytes[0] === 0x01 ? 'ir' : 'cl';
            const canvas    = this.canvasRefs[channelId];
            if (!canvas) return;

            const jpeg = data.slice(1);
            const blob = new Blob([jpeg], { type: 'image/jpeg' });
            const url  = URL.createObjectURL(blob);
            const img  = new Image();
            img.onload = () => {
                canvas.width  = img.width;
                canvas.height = img.height;
                canvas.getContext('2d').drawImage(img, 0, 0);
                URL.revokeObjectURL(url);
            };
            img.src = url;
        },
    },
};
</script>
