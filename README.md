# Exemplo de Integração CPU-CLA no TMS320F28379D

Este projeto demonstra um exemplo prático de configuração e troca de dados entre a CPU principal (CPU1) e o Control Law Accelerator (CLA) no microcontrolador C2000 TMS320F28379D, utilizando a ferramenta gráfica **SysConfig** e o framework **DriverLib**.

## Funcionamento do Exemplo

O fluxo básico de processamento consiste em:
1. **CPU1** escreve um valor flutuante na variável de entrada (`fVal`) localizada na RAM de mensagem de CPU para CLA (`CpuToCla1MsgRAM`).
2. **CPU1** força o disparo da **Task 1** do CLA através da função `CLA_forceTasks(myCLA0_BASE, CLA_TASKFLAG_1)`.
3. O **CLA** processa a `Cla1Task1()`, lê o valor atual de `fVal`, incrementa em 1, realiza um pequeno loop de delay e salva o resultado na variável `fResult` dentro da RAM de mensagem oposta (`Cla1ToCpuMsgRAM`).
4. Ao encerrar a tarefa, uma interrupção de fim de tarefa do CLA (`cla1Isr1`) pode ser gerada na CPU1 para ler o resultado de volta.

## Estrutura de Memória Compartilhada

Para que as variáveis globais sejam acessadas por ambos os núcleos, elas são explicitamente mapeadas em seções específicas de memória de mensagem (Message RAMs) através de pragmas no arquivo `main.c`:

```c
#pragma DATA_SECTION(fVal,"CpuToCla1MsgRAM");
float fVal;

#pragma DATA_SECTION(fResult,"Cla1ToCpuMsgRAM");
float fResult;

```

## Configuração Crítica no SysConfig

O gerenciamento de quais blocos de memória RAM local (`RAMLSx`) pertencem ao CLA ou à CPU e o mapeamento das seções do Linker são feitos graficamente no **SysConfig** (`cla_example.syscfg`).

### 1. Módulo Memory Configuration (`memcfg`)

Para este exemplo funcionar, os blocos de memória RAM local e de mensagens foram configurados da seguinte forma:

* **Mensagens Inicializadas**: Ative as opções para inicializar as seções `MSGCPUTOCLA1` e `MSGCLA1TOCPU`.
* **RAMLS1 (`CLA_data`)**: Configurado como memória de dados do CLA (para alocação de constantes, scratchpad e variáveis internas do compilador CLA C).
* **RAMLS4 e RAMLS5 (`CLA_prog`)**: Configurados como memória de execução de programa do CLA, permitindo que o CLA execute as instruções contidas no arquivo `tasks.cla`.

### 2. Módulo Linker Command Tool (`CMD`)

O SysConfig moderno possui o utilitário `CMD` para definir o mapeamento padrão de seções que o compilador usará para gerar o binário:

* **Seção de Programa (`cla1Prog`)**: Mapeada para o bloco `RAMLS5`.
* **Tabela de Mensagens CPU -> CLA (`sectionMemory_cpuToCla1MsgRAM`)**: Mapeada para `CPUTOCLA_MSGRAM`.
* **Tabela de Mensagens CLA -> CPU (`sectionMemory_cla1ToCpuMsgRAM`)**: Mapeada para `CLATOCPU_MSGRAM`.
* **Scratchpad e Constantes (`claScratchpad` / `claConst`)**: Direcionados para a `RAMLS1`.

---

## Sincronização entre SysConfig e Arquivos .cmd

> ⚠️ **ATENÇÃO - PONTO CRÍTICO DE CONFIGURAÇÃO**
> O SysConfig gera as definições lógicas das seções, mas as propriedades do projeto determinam como os arquivos de Linker Command (`.cmd`) físicos se comportam na compilação em **RAM** ou **FLASH**.

O projeto disponibiliza dois arquivos `.cmd` dependendo do seu *Build Configuration*:

### Cenário A: Execução em RAM (`2837xD_RAM_CLA_lnk_cpu1.cmd`)

Em modo de depuração puramente em RAM, o código do CLA é carregado e executado diretamente nos blocos locais:

```linker
/* Alocação direta para execução */
Cla1Prog         : > RAMLS4, PAGE=0
Cla1ToCpuMsgRAM  : > CLA1_MSGRAMLOW,   PAGE = 1
CpuToCla1MsgRAM  : > CLA1_MSGRAMHIGH,  PAGE = 1

```

### Cenário B: Execução em FLASH (`2837xD_FLASH_CLA_lnk_cpu1.cmd`)

Em modo de produção/Flash, o código de controle do CLA **deve ser armazenado na Flash não-volátil, mas copiado para a RAM em tempo de execução**, pois o CLA não possui barramento físico para ler diretamente da memória Flash.

O arquivo `.cmd` de Flash resolve isso através dos operadores `LOAD`, `RUN` e diretivas de ponteiro de endereço:

```linker
/* CLA específico para Flash */
Cla1Prog    : LOAD = FLASHD,                  /* Armazenado na Flash Setor D */
              RUN = RAMLS5,                   /* Copiado e Executado na RAM LS5 */
              LOAD_START(Cla1funcsLoadStart),
              LOAD_END(Cla1funcsLoadEnd),
              RUN_START(Cla1funcsRunStart),
              LOAD_SIZE(Cla1funcsLoadSize),
              PAGE = 0, ALIGN(8)

```

* **Ajuste Manual Obrigatório**: No código de inicialização da CPU (`main.c` / rotinas de setup), você deve explicitamente utilizar as variáveis geradas pelo linker (`Cla1funcsLoadStart`, `Cla1funcsRunStart`, `Cla1funcsLoadSize`) junto com a função `memcpy()` para mover o código da Flash para a RAM antes de inicializar o módulo do CLA.

### Requisito do Compilador CLA C (`CLA_C`)

Ambos os arquivos `.cmd` possuem tratamento condicional para o compilador CLA C. Certifique-se de que a flag `CLA_C` esteja definida nas propriedades de pré-processamento do Linker no Code Composer Studio:
`Project Properties -> C2000 Linker -> Advanced Options -> Command File Preprocessing -> --define=CLA_C`

Isso garante a correta alocação do espaço de pilha local (Scratchpad):

```linker
#ifdef CLA_C
   CLA_SCRATCHPAD_SIZE = 0x100;
   /* Mapeamento de variáveis locais e temporárias */
   .scratchpad      : > RAMLS1,       PAGE = 0
   .bss_cla		    : > RAMLS1,       PAGE = 0
#endif

```

## Como Executar

1. Abra o projeto no **Code Composer Studio (CCS)**.
2. Selecione o perfil desejado (`RAM` ou `FLASH`) clicando com o botão direito no projeto -> *Build Configurations* -> *Set Active*.
3. Verifique se o arquivo `.syscfg` não aponta conflitos de memória com os blocos declarados no `.cmd`.
4. Compile o projeto (*Build*).
5. Inicie a sessão de Debug e adicione as variáveis `fVal` e `fResult` à janela de *Expressions* para acompanhar a soma incremental em tempo real.


