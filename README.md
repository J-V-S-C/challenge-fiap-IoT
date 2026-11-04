# Simulador clínico — API em C++

## Sobre o projeto

O projeto permite que estudantes pratiquem atendimentos clínicos por meio de uma simulação textual e cronometrada.

O estudante envia condutas para um paciente virtual. O sistema avalia cada interação, avança o tempo e atualiza os sintomas e sinais vitais. A simulação termina quando o estudante conclui o atendimento, o tempo acaba ou o paciente atinge um estado crítico.

## Simplificação do projeto original

Este módulo C++ é uma versão simplificada do simulador original desenvolvido em Python.

O projeto original possui protocolos, denúncias e integração com uma LLM. Esta versão C++ implementa o fluxo principal com dois casos selecionáveis, evolução clínica simulada, histórico de interações e persistência local em SQLite.

O objetivo não é substituir o backend Python, mas demonstrar de forma simples uma API em C++20.

## Fluxo

1. consultar os casos e escolher um;
2. iniciar uma simulação com limite de tempo;
3. enviar condutas e consultar a evolução do paciente e o histórico;
4. encerrar e visualizar o resultado calculado pelo servidor.

O banco SQLite preserva as simulações após reiniciar o servidor. Um mock baseado em palavras-chave é usado no lugar de uma LLM.

## API

URL base: `http://localhost:8080/api/v1`

| Método | Rota                             | Função                                     |
| ------ | -------------------------------- | ------------------------------------------ |
| `GET`  | `http://localhost:8080/`        | Mostra as rotas e exemplos de uso          |
| `GET`  | `/health`                       | Confirma que o servidor está disponível    |
| `GET`  | `/cases`                        | Lista os dois casos clínicos               |
| `GET`  | `/cases/{id}`                   | Consulta um caso e o estado inicial        |
| `POST` | `/simulations`                   | Inicia uma simulação                       |
| `GET`  | `/simulations`                  | Lista tentativas salvas                    |
| `GET`  | `/simulations/{id}`              | Consulta o estado atual da simulação       |
| `POST` | `/simulations/{id}/interactions` | Processa uma conduta e atualiza o paciente |
| `POST` | `/simulations/{id}/finish`       | Encerra a simulação                        |
| `GET`  | `/simulations/{id}/result`       | Consulta o resultado após o encerramento   |

Ao abrir `http://localhost:8080` no navegador, a API mostra as rotas disponíveis, os campos esperados e exemplos de preenchimento.

### Iniciar

```json
{
  "time_limit_minutes": 30,
  "case_id": 1
}
```

`case_id` pode ser `1` (dor torácica) ou `2` (desidratação). Se for omitido, usa o caso `1` para manter compatibilidade com a versão anterior.

### Interagir

```json
{
  "student_text": "Administer oxygen and monitor saturation.",
  "elapsed_minutes": 2
}
```

A resposta contém a avaliação da conduta, a resposta do paciente, o tempo decorrido e o estado clínico atualizado. O campo `history` guarda as interações anteriores. O caso 1 reconhece condutas relacionadas a oxigênio, saturação e alívio da dor; o caso 2 reconhece água, hidratação e fluidos. São regras demonstrativas, sem avaliação clínica por IA.

### Encerrar

Envie `POST /simulations/{id}/finish` sem campos obrigatórios.

A resposta contém o estado final e um resultado calculado pelo servidor: tempo utilizado, quantidade de interações, condutas consideradas corretas e percentual de acerto. O desfecho também considera fim por tempo ou estado crítico; o cliente não escolhe o resultado.

O resultado também fica disponível em `GET /simulations/{id}/result`. Enquanto a simulação estiver ativa, essa consulta retorna `409`.

## Requisições prontas com HTTPie

Com o servidor em execução, copie e cole os comandos na ordem abaixo.

### Ver as rotas

```bash
http GET localhost:8080/
http GET localhost:8080/api/v1/health
http GET localhost:8080/api/v1/cases
```

### 1. Iniciar a simulação

```bash
http POST localhost:8080/api/v1/simulations time_limit_minutes:=30 case_id:=1
```

A resposta retorna um `simulation_id`. Nos próximos comandos, substitua `1` pelo ID recebido caso ele seja diferente.

### 2. Enviar uma interação

```bash
http POST localhost:8080/api/v1/simulations/1/interactions \
  student_text="Administer oxygen and monitor saturation." \
  elapsed_minutes:=2
```

Para consultar o estado atualizado, use `http GET localhost:8080/api/v1/simulations/1`.

### 3. Encerrar a simulação

```bash
http POST localhost:8080/api/v1/simulations/1/finish
```

Para consultar o resultado, use `http GET localhost:8080/api/v1/simulations/1/result`. A lista de tentativas fica em `http GET localhost:8080/api/v1/simulations`.

No HTTPie, `=` envia texto e `:=` envia um valor JSON, como o número `30`. Não passe o objeto JSON sem aspas diretamente no terminal, pois o shell separa seu conteúdo e a API recebe um corpo inválido.

## Consumir a API com Python

Com o servidor C++ rodando em outro terminal, execute:

```bash
python3 examples/consume_api.py
```

O script usa apenas a biblioteca padrão de Python e demonstra o ciclo HTTP completo com o caso 2: listar casos, iniciar, interagir, consultar estado, encerrar e consultar resultado. Cada execução usa o ID retornado pelo servidor.

## Comportamento e limites

- O SQLite é criado em `data/clinical_simulator.db`. Para outro caminho, defina `CLINICAL_SIMULATOR_DB=/caminho/arquivo.db` ao iniciar. A pasta `data/` e o banco são ignorados pelo Git.
- O estado completo, o histórico e o resultado permanecem disponíveis depois de reiniciar o servidor com o mesmo banco.
- A API C++ roda em `127.0.0.1:8080`, sem autenticação e destinada à demonstração local.
- `time_limit_minutes` e `elapsed_minutes` devem ser inteiros positivos. JSON inválido recebe `400`, ID inexistente recebe `404` e ações em simulações encerradas recebem `409`.
- O frontend principal do Challenge usa o backend Python. Este módulo é a alternativa C++ local proposta para a disciplina de Edge, com seu próprio contrato de API.

## Tecnologias

- C++20;
- CMake 3.15 ou superior;
- `cpp-httplib`;
- `nlohmann/json`.
- SQLite3 (biblioteca de desenvolvimento instalada no sistema).

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

O `cpp-httplib` acompanha o projeto. Se o `nlohmann/json` não estiver instalado, o CMake baixa a versão 3.11.3 na primeira configuração. O CMake precisa encontrar SQLite3 no sistema.
