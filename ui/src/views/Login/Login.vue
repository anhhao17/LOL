<template>
    <div class="login-page">
        <div class="login-card">
            <div class="login-logo">
                <div class="logo-mark">Q</div>
                <h1>QUANTUM</h1>
                <p>System Management Interface</p>
            </div>

            <form @submit.prevent="submit">
                <div class="mb-3">
                    <label for="username" class="form-label">Username</label>
                    <input
                        id="username"
                        v-model="form.username"
                        type="text"
                        class="form-control"
                        autocomplete="username"
                        :disabled="loading"
                        required
                    />
                </div>

                <div class="mb-3">
                    <label for="password" class="form-label">Password</label>
                    <div class="password-wrapper">
                        <input
                            id="password"
                            v-model="form.password"
                            :type="showPassword ? 'text' : 'password'"
                            class="form-control"
                            autocomplete="current-password"
                            :disabled="loading"
                            required
                        />
                        <button
                            type="button"
                            class="toggle-password"
                            :aria-label="showPassword ? 'Hide password' : 'Show password'"
                            @click="showPassword = !showPassword"
                        >
                            {{ showPassword ? '&#128065;' : '&#128274;' }}
                        </button>
                    </div>
                </div>

                <button type="submit" class="btn-login" :disabled="loading">
                    {{ loading ? 'Signing in…' : 'Sign in' }}
                </button>

                <div v-if="error" class="error-alert" role="alert">
                    {{ error }}
                </div>
            </form>
        </div>
    </div>
</template>

<script>
export default {
    name: 'LoginView',
    data() {
        return {
            form:         { username: '', password: '' },
            showPassword: false,
            loading:      false,
            error:        null,
        };
    },
    methods: {
        async submit() {
            this.error   = null;
            this.loading = true;
            try {
                await this.$store.dispatch('authentication/login', this.form);
                this.$router.push({ name: 'stream' });
            } catch (err) {
                this.error = err?.response?.data?.error?.message || 'Login failed. Please try again.';
            } finally {
                this.loading = false;
            }
        },
    },
};
</script>
