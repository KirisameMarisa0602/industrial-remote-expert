TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    common \
    server \
    client-expert \
    client-factory \
    shared

# Define dependencies
server.depends = common
client-expert.depends = common shared
client-factory.depends = common shared

# Shared UI components
shared.subdir = shared
shared.target = shared