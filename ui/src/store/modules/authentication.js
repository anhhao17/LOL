import axios from 'axios';

// Matches webui-vue store/modules/authentication.js pattern.
const state = {
    username:   null,
    role:       null,
    csrfToken:  null,
};

const getters = {
    isAuthenticated: (state) => !!state.csrfToken,
    username:        (state) => state.username,
    role:            (state) => state.role,
    csrfToken:       (state) => state.csrfToken,
};

const mutations = {
    setSession(state, { username, role, csrfToken }) {
        state.username  = username;
        state.role      = role;
        state.csrfToken = csrfToken;
    },
    clearSession(state) {
        state.username  = null;
        state.role      = null;
        state.csrfToken = null;
    },
};

const actions = {
    async login({ commit }, { username, password }) {
        const response = await axios.post('/api/v1/login', { username, password });
        const { csrfToken, role } = response.data.data;
        commit('setSession', { username, role, csrfToken });
        // Attach CSRF token to all future requests.
        axios.defaults.headers.common['X-CSRF-Token'] = csrfToken;
    },

    async logout({ commit }) {
        try {
            await axios.post('/api/v1/logout');
        } catch {
            // Session may already be gone — clear client state regardless.
        }
        delete axios.defaults.headers.common['X-CSRF-Token'];
        commit('clearSession');
    },
};

export default {
    namespaced: true,
    state,
    getters,
    mutations,
    actions,
};
