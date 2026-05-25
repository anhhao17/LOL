<template>
    <nav class="app-nav">
        <div class="nav-brand">
            <div class="brand-name">Quantum</div>
            <div class="brand-sub">System Management</div>
        </div>

        <ul class="nav flex-column">
            <li class="nav-item">
                <router-link :to="{ name: 'stream' }">
                    <span class="nav-icon">&#9654;</span>
                    Live Stream
                </router-link>
            </li>
        </ul>

        <div class="nav-spacer" />

        <div class="nav-footer">
            <button class="btn-sm-outline w-100" @click="logout">
                Sign out
            </button>
        </div>
    </nav>
</template>

<script>
export default {
    name: 'AppNavigation',
    methods: {
        async logout() {
            await this.$store.dispatch('authentication/logout');
            this.$router.push({ name: 'login' });
        },
    },
};
</script>
