#!/usr/bin/env bash
scriptpath="$( cd "$(dirname "$0")" ; pwd -P )"

cd "$scriptpath/.."

docker build -f ./.devcontainer/Dockerfile --tag=reg.choncholas.com/research/mpc3-dev:latest .
docker push reg.choncholas.com/research/mpc3-dev:latest || echo "Skipping push"
docker build -f ./docker/Dockerfile --tag=reg.choncholas.com/research/mpc3:latest .
docker push reg.choncholas.com/research/mpc3:latest || echo "Skipping push"