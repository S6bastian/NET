#!/bin/bash
FOLDER="${1:-$PWD}"
SESSION="dev_socket_session"

tmux kill-session -t $SESSION 2>/dev/null

# Usamos comillas simples '' para el bloque de bash -c
gnome-terminal --working-directory="$FOLDER" -- bash -c '
  tmux new-session -d -s '$SESSION' -c "'$FOLDER'";
  tmux set -g mouse on;

  # Atajos Alt+Flechas
  tmux bind -n M-Left select-pane -L;
  tmux bind -n M-Right select-pane -R;
  tmux bind -n M-Up select-pane -U;
  tmux bind -n M-Down select-pane -D;

  # Crear cuadricula
  tmux split-window -v -c "'$FOLDER'";
  tmux split-window -h -c "'$FOLDER'";
  tmux select-pane -t 0;
  tmux split-window -h -c "'$FOLDER'";
  tmux select-layout tiled;

  # Definir Alias (se envían primero)
  tmux send-keys -t 0 "alias s=\"g++ server.cpp -o server && ./server\"" Enter;
  tmux send-keys -t 1 "alias c=\"g++ client.cpp -o client && ./client\"" Enter;

  # Enviar comandos con Enter explícito
  tmux send-keys -t 0 "s" Enter;
  tmux send-keys -t 1 "sleep 3 && c" Enter;
  tmux send-keys -t 2 "sleep 4 && ./client" Enter;
  tmux send-keys -t 3 "sleep 5 && ./client" Enter;

  tmux attach-session -t '$SESSION';
'