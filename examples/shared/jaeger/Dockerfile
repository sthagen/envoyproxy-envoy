FROM jaegertracing/all-in-one@sha256:e9bfb797d7af61bd6bd9647efec960782dbbfa77d321d0368a39f93482aff061
HEALTHCHECK \
    --interval=1s \
    --timeout=1s \
    --start-period=1s \
    --retries=60 \
    CMD wget -q --header='Content-Type:application/json' -O - http://localhost:14269/health | grep "Server available"
