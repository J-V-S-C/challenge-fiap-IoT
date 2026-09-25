# Simulador clínico — API em C++

## Sobre o projeto

O projeto permite que estudantes pratiquem atendimentos clínicos por meio de uma simulação textual e cronometrada.

O estudante envia condutas para um paciente virtual. O sistema avalia cada interação, avança o tempo e atualiza os sintomas e sinais vitais. A simulação termina quando o estudante conclui o atendimento, o tempo acaba ou o paciente atinge um estado crítico.

## Simplificação do projeto original

Este módulo C++ é uma versão simplificada do simulador original desenvolvido em Python.

O projeto original possui seleção de casos, protocolos, denúncias, histórico completo, avanço de tempo separado e mais informações no resultado. Esta versão implementa somente o fluxo principal com um caso clínico fixo e três rotas de negócio.

O objetivo não é substituir o backend Python, mas demonstrar de forma simples uma API em C++20.

## Fluxo

1. iniciar uma simulação;
2. interagir com o paciente;
3. encerrar e visualizar o resultado.

As simulações ficam armazenadas em memória enquanto o servidor estiver em execução. Um mock baseado em palavras-chave é usado no lugar de uma LLM.

## API

URL base: `http://localhost:8080/api/v1`

| Método | Rota                             | Função                                     |
| ------ | -------------------------------- | ------------------------------------------ |
| `GET`  | `/`                             | Mostra as rotas e exemplos de uso          |
| `POST` | `/simulations`                   | Inicia uma simulação                       |
| `POST` | `/simulations/{id}/interactions` | Processa uma conduta e atualiza o paciente |
| `POST` | `/simulations/{id}/finish`       | Encerra a simulação                        |

Ao abrir `http://localhost:8080` no navegador, a API mostra as rotas disponíveis, os campos esperados e exemplos de preenchimento.

### Iniciar

```json
{
  "time_limit_minutes": 30
}
```

### Interagir

```json
{
  "student_text": "Administer oxygen and monitor saturation.",
  "elapsed_minutes": 2
}
```

A resposta contém a avaliação da conduta, a resposta do paciente, o tempo decorrido e o estado clínico atualizado.

### Encerrar

```json
{
  "outcome": "Care completed"
}
```

A resposta contém o estado final, o tempo utilizado e a quantidade de interações.

## Requisições prontas com HTTPie

Com o servidor em execução, copie e cole os comandos na ordem abaixo.

### Ver as rotas

```bash
http GET localhost:8080/
```

### 1. Iniciar a simulação

```bash
http POST localhost:8080/api/v1/simulations time_limit_minutes:=30
```

A resposta retorna um `simulation_id`. Nos próximos comandos, substitua `1` pelo ID recebido caso ele seja diferente.

### 2. Enviar uma interação

```bash
http POST localhost:8080/api/v1/simulations/1/interactions \
  student_text="Administer oxygen and monitor saturation." \
  elapsed_minutes:=2
```

### 3. Encerrar a simulação

```bash
http POST localhost:8080/api/v1/simulations/1/finish \
  outcome="Care completed"
```

No HTTPie, `=` envia texto e `:=` envia um valor JSON, como o número `30`. Não passe o objeto JSON sem aspas diretamente no terminal, pois o shell separa seu conteúdo e a API recebe um corpo inválido.

## Tecnologias

- C++20;
- CMake 3.15 ou superior;
- `cpp-httplib`;
- `nlohmann/json`.

## Executar

```bash
cmake -S . -B build
cmake --build build
./build/clinical_simulator
```

Também é possível usar:

```bash
./build_and_run.sh
```

O `cpp-httplib` acompanha o projeto. Se o `nlohmann/json` não estiver instalado, o CMake baixa a versão 3.11.3 na primeira configuração.
