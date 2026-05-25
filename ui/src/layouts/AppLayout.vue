<template>
    <div class="app-shell">
        <app-navigation />
        <div class="app-content">
            <app-header />
            <main class="page-container">
                <router-view />
            </main>
        </div>
    </div>
</template>

<script>
import AppNavigation from '@/components/AppNavigation/AppNavigation.vue';
import AppHeader     from '@/components/AppHeader/AppHeader.vue';

export default {
    name: 'AppLayout',
    components: { AppNavigation, AppHeader },
};
</script>
