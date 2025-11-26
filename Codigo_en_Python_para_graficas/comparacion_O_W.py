import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.lines import Line2D

# Definir nombres de archivos
file_o = 'DATOS_BUENOS_O.txt'
file_w = 'DATOS_BUENOS_W.txt'

try:
    # Cargar datos (sin cabecera, separados por tabulaciones)
    df_o = pd.read_csv(file_o, sep='\t', header=None)
    df_w = pd.read_csv(file_w, sep='\t', header=None)

    # Extraer datos del Run 0 (primera fila, índice 0)
    run_index = 0
    
    # Datos para Variable O (10x10)
    # Columnas: 0=N, 1=Plaq, 2=ErrPlaq, 3=Mean_O, 4=Err_O, ...
    mean_o = df_o.iloc[run_index, 3]
    err_o = df_o.iloc[run_index, 4]

    # Datos para Variable W (10x10)
    # W10 Mean está en columna 19, Error en 20
    mean_w = df_w.iloc[run_index, 19]
    err_w = df_w.iloc[run_index, 20]

    # Configuración general de fuentes
    plt.rcParams.update({'font.size': 14}) # Aumentar tamaño de fuente general

    # Crear figura con un solo plot
    fig, ax1 = plt.subplots(figsize=(10, 8))

    # --- Gráfica 1: Comparación de Promedios ---
    
    # Puntos reales con barras de error
    ax1.errorbar(0, mean_o, yerr=err_o, fmt='o', color='blue', ecolor='blue', 
                 capsize=10, markersize=10, elinewidth=2, markeredgewidth=2)
    ax1.errorbar(1, mean_w, yerr=err_w, fmt='o', color='red', ecolor='red', 
                 capsize=10, markersize=10, elinewidth=2, markeredgewidth=2)

    # Configuración del eje X
    ax1.set_xticks([0, 1])
    ax1.set_xticklabels(['O (Multinivel)', 'W (Estándar)'])
    ax1.set_xlim(-0.5, 1.5)

    # Etiquetas y Título
    ax1.set_ylabel(r'$<A>$', fontsize=18, fontweight='bold') # Etiqueta eje Y solicitada
    ax1.set_title('Comparación operadores O y W en Loop 10x10', fontsize=16, fontweight='bold')
    
    # Grid y Leyenda
    ax1.grid(True, linestyle='--', alpha=0.5)
    
    # Crear handles personalizados para la leyenda
    legend_elements = [
        Line2D([0], [0], marker='o', color='w', label='Variable O', 
               markerfacecolor='blue', markersize=10),
        Line2D([0], [0], marker='o', color='w', label='Variable W', 
               markerfacecolor='red', markersize=10)
    ]
    ax1.legend(handles=legend_elements, loc='best', fontsize=14, title="Operadores")

    plt.tight_layout()
    plt.savefig('comparacion_run0_O_W_single.png', dpi=300)
    
    print(f"Datos Run 0:")
    print(f"O: {mean_o:.8f} +/- {err_o:.8f}")
    print(f"W: {mean_w:.8f} +/- {err_w:.8f}")
    print("Gráfica generada: comparacion_run0_O_W_single.png")

except Exception as e:
    print(f"Error: {e}")
    import traceback
    traceback.print_exc()