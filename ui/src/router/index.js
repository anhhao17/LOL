import { createRouter, createWebHistory } from 'vue-router';
import store from '@/store';
import routes from './routes';

const router = createRouter({
    history: createWebHistory(),
    routes,
});

router.beforeEach((to) => {
    const isAuthenticated = store.getters['authentication/isAuthenticated'];

    if (to.meta.requiresAuth && !isAuthenticated) {
        return { name: 'login' };
    }
    if (to.name === 'login' && isAuthenticated) {
        return { name: 'stream' };
    }
});

export default router;
