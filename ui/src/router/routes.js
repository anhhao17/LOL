import AppLayout   from '@/layouts/AppLayout.vue';
import LoginLayout from '@/layouts/LoginLayout.vue';

export default [
    {
        path: '/login',
        component: LoginLayout,
        children: [
            {
                path: '',
                name: 'login',
                component: () => import('@/views/Login/Login.vue'),
                meta: { requiresAuth: false },
            },
        ],
    },
    {
        path: '/',
        component: AppLayout,
        meta: { requiresAuth: true },
        children: [
            {
                path: '',
                redirect: { name: 'stream' },
            },
            {
                path: 'stream',
                name: 'stream',
                component: () => import('@/views/Stream/Stream.vue'),
                meta: { requiresAuth: true, title: 'Live Stream' },
            },
        ],
    },
    {
        path: '/:pathMatch(.*)*',
        redirect: '/',
    },
];
