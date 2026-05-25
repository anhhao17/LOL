<template>
    <header class="app-header">
        <span class="header-title">{{ pageTitle }}</span>
        <div class="header-actions">
            <div class="user-badge">
                <div class="user-avatar">{{ initials }}</div>
                <span>{{ username }}</span>
            </div>
        </div>
    </header>
</template>

<script>
import { mapGetters } from 'vuex';

export default {
    name: 'AppHeader',
    computed: {
        ...mapGetters('authentication', ['username']),
        initials() {
            return (this.username || '?').charAt(0).toUpperCase();
        },
        pageTitle() {
            return this.$route.meta.title || 'Quantum';
        },
    },
};
</script>
