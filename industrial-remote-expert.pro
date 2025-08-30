TEMPLATE = subdirs

SUBDIRS = server \
          client-expert \
          client-factory \
          launcher

# Ensure server builds first
client-expert.depends = server
client-factory.depends = server
launcher.depends = server

# Set build order
CONFIG += ordered