FROM jaegertracing/all-in-one@sha256:93956ad8839cd240587b897eba487df6d47b17e346c6dcadce5bb866a00e2817
HEALTHCHECK \
    --interval=1s \
    --timeout=1s \
    --start-period=1s \
    --retries=60 \
    CMD wget -q --header='Content-Type:application/json' -O - http://localhost:14269/health | grep "Server available"
