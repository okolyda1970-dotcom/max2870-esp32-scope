package com.max2870.controller;

import javax.swing.*;
import javax.swing.border.EmptyBorder;
import java.awt.*;

public class Max2870Gui extends JFrame implements EspConnection.ConnectionListener {

    private static final long serialVersionUID = 1L;

    private EspConnection connection;
    
    // --- UI Components ---
    private JComboBox<String> comboPorts;
    private JButton btnConnect;
    private JButton btnRefresh;
    
    private JTextField txtFreq;
    private JSlider sliderTune;
    private JTextField txtTune;
    
    private JComboBox<Integer> cbPower;
    private JComboBox<Integer> cbCpCurrent;
    private JComboBox<String> cbLdMode;
    private JButton btnApply;
    
    private JLabel lblStatus;
    private JTextArea textLog;
    
    // Scope Components
    private OscilloscopePanel scopePanel;
    private JButton btnStartScope;
    private JButton btnStopScope;
    
    // Scope Control Panel Components
    private JSlider sldTimeDiv;
    private JSlider sldVoltDiv;
    private JSpinner spinPosX;
    private JSpinner spinPosY;
    private JSlider sldTrigger;
    private JButton btnPauseResume;
    private JCheckBox chkAutoScale;

    public Max2870Gui() {
        super("MAX2870 Controller & Scope via ESP32");
        setDefaultCloseOperation(EXIT_ON_CLOSE);
        setSize(1000, 800); 
        setLocationRelativeTo(null);

        connection = new EspConnection(this);
        
        initComponents();
        refreshPorts();
    }

    private void initComponents() {
        setLayout(new BorderLayout(5, 5));

        // ==========================================
        // 1. TOP PANEL: Ports & Scope Buttons
        // ==========================================
        JPanel topPanel = new JPanel(new FlowLayout(FlowLayout.LEFT, 8, 6));
        topPanel.setBorder(new EmptyBorder(6, 8, 6, 8));
        
        topPanel.add(new JLabel("COM Port:"));
        comboPorts = new JComboBox<>();
        comboPorts.setPreferredSize(new Dimension(140, 26));
        topPanel.add(comboPorts);

        btnRefresh = new JButton("Refresh");
        btnRefresh.addActionListener(e -> refreshPorts());
        topPanel.add(btnRefresh);

        btnConnect = new JButton("Connect");
        btnConnect.setBackground(new Color(0, 150, 0));
        btnConnect.setForeground(Color.WHITE);
        btnConnect.addActionListener(e -> toggleConnection());
        topPanel.add(btnConnect);
        
        topPanel.add(Box.createHorizontalStrut(20));
        
        btnStartScope = new JButton("▶ START SCOPE");
        btnStartScope.setEnabled(false);
        btnStartScope.addActionListener(e -> {
            connection.startScopeStream();
            btnStartScope.setEnabled(false);
            btnStopScope.setEnabled(true);
        });
        topPanel.add(btnStartScope);

        btnStopScope = new JButton("⏹ STOP SCOPE");
        btnStopScope.setEnabled(false);
        btnStopScope.addActionListener(e -> {
            connection.stopScopeStream();
            btnStopScope.setEnabled(false);
            btnStartScope.setEnabled(true);
        });
        topPanel.add(btnStopScope);

        add(topPanel, BorderLayout.NORTH);

        // ==========================================
        // 2. CENTER PANEL: Frequency & PLL Settings
        // ==========================================
        JPanel centerWrapper = new JPanel(new GridLayout(2, 1, 10, 10)); 
        centerWrapper.setBorder(new EmptyBorder(10, 10, 10, 10));

        // --- Part A: Manual Frequency Control ---
        JPanel freqPanel = createManualFreqPanel();
        centerWrapper.add(freqPanel);

        // --- Part B: Advanced PLL Settings ---
        JPanel advPanel = createAdvancedSettingsPanel();
        centerWrapper.add(advPanel);
        
        add(centerWrapper, BorderLayout.CENTER);

        // ==========================================
        // 3. SOUTH PANEL: Status, Scope Split, Log
        // ==========================================
        JPanel southContainer = new JPanel(new BorderLayout(5, 5));
        southContainer.setBorder(new EmptyBorder(0, 10, 10, 10));

        lblStatus = new JLabel(" DISCONNECTED ");
        lblStatus.setOpaque(true);
        lblStatus.setBackground(Color.GRAY);
        lblStatus.setForeground(Color.WHITE);
        lblStatus.setHorizontalAlignment(SwingConstants.CENTER);
        lblStatus.setPreferredSize(new Dimension(0, 25));
        southContainer.add(lblStatus, BorderLayout.NORTH);

        // SplitPane for Graph and Controls
        JSplitPane scopeSplit = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT);
        scopeSplit.setResizeWeight(0.75); 
        
        scopePanel = new OscilloscopePanel();
        scopePanel.setPreferredSize(new Dimension(400, 200));
        scopeSplit.setLeftComponent(scopePanel);
        
        JPanel scopeControls = createScopeControlPanel();
        scopeSplit.setRightComponent(scopeControls);
        
        southContainer.add(scopeSplit, BorderLayout.CENTER);

        textLog = new JTextArea();
        textLog.setEditable(false);
        textLog.setFont(new Font("Monospaced", Font.PLAIN, 10));
        JScrollPane logScroll = new JScrollPane(textLog);
        logScroll.setPreferredSize(new Dimension(0, 80));
        southContainer.add(logScroll, BorderLayout.SOUTH);

        add(southContainer, BorderLayout.SOUTH);
    }

    // --- Helper Methods for Panels ---

    private JPanel createManualFreqPanel() {
        JPanel panel = new JPanel(new GridBagLayout());
        GridBagConstraints gc = new GridBagConstraints();
        gc.insets = new Insets(4, 4, 4, 4);
        gc.anchor = GridBagConstraints.WEST;

        // Строка 0: Поле ввода частоты
        gc.gridx = 0; gc.gridy = 0;
        panel.add(new JLabel("Frequency (MHz):"), gc);
        
        gc.gridx = 1; gc.gridy = 0;
        txtFreq = new JTextField("1000.00", 12); // Сделали чуть шире
        txtFreq.setFont(new Font("Monospaced", Font.BOLD, 14)); // Крупный шрифт для читаемости
        panel.add(txtFreq, gc);

        // Строка 1: Кнопки быстрой подстройки (Шаги)
        // Логика: Нажатие меняет текст в txtFreq
        
        gc.gridx = 0; gc.gridy = 1; gc.gridwidth = 2;
        JPanel stepButtonsPanel = new JPanel(new FlowLayout(FlowLayout.CENTER, 5, 5));
        
        // Функция-помощник для создания кнопок, меняющих частоту
        stepButtonsPanel.add(createStepButton("-1 MHz", -1.0f));
        stepButtonsPanel.add(createStepButton("-100 kHz", -0.1f));
        stepButtonsPanel.add(createStepButton("-10 kHz", -0.01f));
        stepButtonsPanel.add(createStepButton("-1 kHz", -0.001f));
        
        stepButtonsPanel.add(Box.createHorizontalStrut(10)); // Разделитель
        
        stepButtonsPanel.add(createStepButton("+1 kHz", +0.001f));
        stepButtonsPanel.add(createStepButton("+10 kHz", +0.01f));
        stepButtonsPanel.add(createStepButton("+100 kHz", +0.1f));
        stepButtonsPanel.add(createStepButton("+1 MHz", +1.0f));

        panel.add(stepButtonsPanel, gc);

        return panel;
    }

    /**
     * Создает кнопку, которая прибавляет/вычитает delta к текущему значению в txtFreq
     */
    private JButton createStepButton(String label, float delta) {
        JButton b = new JButton(label);
        b.addActionListener(e -> {
            try {
                // Читаем текущее значение из поля
                String text = txtFreq.getText().trim().replace(',', '.');
                float currentVal = Float.parseFloat(text);
                
                // Меняем значение
                float newVal = currentVal + delta;
                
                // Ограничения диапазона MAX2870 (примерно 23.5 .. 6000 МГц)
                if (newVal < 23.5f) newVal = 23.5f;
                if (newVal > 6000.0f) newVal = 6000.0f;
                
                // Записываем обратно в поле с форматированием (2 знака после запятой)
                txtFreq.setText(String.format(java.util.Locale.US, "%.2f", newVal));
                
            } catch (NumberFormatException ex) {
                // Если в поле мусор, ставим дефолт или игнорируем
                JOptionPane.showMessageDialog(this, "Некорректное значение частоты!", "Ошибка", JOptionPane.WARNING_MESSAGE);
            }
        });
        return b;
    }

    private JPanel createAdvancedSettingsPanel() {
        JPanel panel = new JPanel(new FlowLayout(FlowLayout.LEFT, 15, 10));
        panel.setBorder(BorderFactory.createTitledBorder("Advanced PLL Settings"));

        panel.add(new JLabel("Output Power Code:"));
        cbPower = new JComboBox<>(new Integer[]{0, 1, 2, 3});
        cbPower.setSelectedItem(3);
        panel.add(cbPower);

        panel.add(new JLabel("CP Current Code:"));
        cbCpCurrent = new JComboBox<>();
        for(int i=0; i<=15; i++) cbCpCurrent.addItem(i);
        cbCpCurrent.setSelectedItem(4);
        panel.add(cbCpCurrent);

        panel.add(new JLabel("LD Pin Function:"));
        cbLdMode = new JComboBox<>(new String[]{"Low (Constant 0)", "Digital Lock Detect", "Analog Lock Detect", "High (Constant 1)"});
        cbLdMode.setSelectedIndex(1);
        panel.add(cbLdMode);

        btnApply = new JButton("APPLY ALL SETTINGS");
        btnApply.setFont(btnApply.getFont().deriveFont(Font.BOLD));
        btnApply.setEnabled(false);
        btnApply.addActionListener(e -> sendAll());
        panel.add(btnApply);

        return panel;
    }
    
    private JPanel createScopeControlPanel() {
        JPanel panel = new JPanel(new GridBagLayout());
        panel.setBorder(BorderFactory.createTitledBorder("Scope Controls"));
        GridBagConstraints gc = new GridBagConstraints();
        gc.insets = new Insets(2, 5, 2, 5);
        gc.anchor = GridBagConstraints.WEST;
        gc.fill = GridBagConstraints.HORIZONTAL;

        // 1. Time/Div
        gc.gridx = 0; gc.gridy = 0;
        panel.add(new JLabel("Time/Div:"), gc);
        gc.gridx = 1; gc.gridy = 0;
        sldTimeDiv = new JSlider(1, 100, 10); 
        sldTimeDiv.addChangeListener(e -> scopePanel.setTimePerDiv(sldTimeDiv.getValue()));
        panel.add(sldTimeDiv, gc);

        // 2. Volt/Div
        gc.gridx = 0; gc.gridy = 1;
        panel.add(new JLabel("Volt/Div:"), gc);
        gc.gridx = 1; gc.gridy = 1;
        sldVoltDiv = new JSlider(10, 1000, 100);
        sldVoltDiv.addChangeListener(e -> scopePanel.setVoltPerDiv(sldVoltDiv.getValue()));
        panel.add(sldVoltDiv, gc);

        // 3. Position X/Y
        gc.gridx = 0; gc.gridy = 2;
        panel.add(new JLabel("Pos X:"), gc);
        gc.gridx = 1; gc.gridy = 2;
        spinPosX = new JSpinner(new SpinnerNumberModel(0, -500, 500, 1));
        spinPosX.addChangeListener(e -> scopePanel.setPosX((Integer)spinPosX.getValue()));
        panel.add(spinPosX, gc);

        gc.gridx = 0; gc.gridy = 3;
        panel.add(new JLabel("Pos Y:"), gc);
        gc.gridx = 1; gc.gridy = 3;
        spinPosY = new JSpinner(new SpinnerNumberModel(0, -500, 500, 1));
        spinPosY.addChangeListener(e -> scopePanel.setPosY((Integer)spinPosY.getValue()));
        panel.add(spinPosY, gc);

        // 4. Trigger Level
        gc.gridx = 0; gc.gridy = 4;
        panel.add(new JLabel("Trig Lvl:"), gc);
        gc.gridx = 1; gc.gridy = 4;
        sldTrigger = new JSlider(0, 4095, 2048);
        sldTrigger.addChangeListener(e -> scopePanel.setTriggerLevel(sldTrigger.getValue()));
        panel.add(sldTrigger, gc);

        // 5. Auto Scale Checkbox
        gc.gridx = 0; gc.gridy = 5; gc.gridwidth = 2;
        chkAutoScale = new JCheckBox("Auto Scale", true);
        chkAutoScale.addActionListener(e -> scopePanel.setAutoScale(chkAutoScale.isSelected()));
        panel.add(chkAutoScale, gc);

        // 6. Pause Button
        gc.gridx = 0; gc.gridy = 6; gc.gridwidth = 2;
        btnPauseResume = new JButton("⏸ PAUSE");
        btnPauseResume.setBackground(new Color(200, 150, 0));
        btnPauseResume.addActionListener(e -> togglePause());
        panel.add(btnPauseResume, gc);

        return panel;
    }
    
    private void togglePause() {
        boolean currentlyPaused = scopePanel.isPaused();
        scopePanel.setPaused(!currentlyPaused);
        
        if (!currentlyPaused) {
            btnPauseResume.setText("▶ RESUME");
            btnPauseResume.setBackground(new Color(0, 150, 0));
        } else {
            btnPauseResume.setText("⏸ PAUSE");
            btnPauseResume.setBackground(new Color(200, 150, 0));
        }
    }

    // --- Logic Implementation ---

    private JButton createTuneButton(String label, int delta) {
        JButton b = new JButton(label);
        b.addActionListener(e -> {
            int newVal = sliderTune.getValue() + delta;
            if (newVal < sliderTune.getMinimum()) newVal = sliderTune.getMinimum();
            if (newVal > sliderTune.getMaximum()) newVal = sliderTune.getMaximum();
            sliderTune.setValue(newVal);
        });
        return b;
    }

    private void syncSliderFromText() {
        try {
            int v = Integer.parseInt(txtTune.getText().trim());
            if (v < sliderTune.getMinimum()) v = sliderTune.getMinimum();
            if (v > sliderTune.getMaximum()) v = sliderTune.getMaximum();
            sliderTune.setValue(v);
        } catch (NumberFormatException ex) {
            txtTune.setText(String.valueOf(sliderTune.getValue()));
        }
    }

    private void refreshPorts() {
        comboPorts.removeAllItems();
        for (String p : connection.getAvailablePorts()) comboPorts.addItem(p);
    }

    private void toggleConnection() {
        if (connection.isOpen()) disconnect();
        else connect();
    }

    private void connect() {
        String port = (String) comboPorts.getSelectedItem();
        if (port == null) {
            JOptionPane.showMessageDialog(this, "Выберите порт", "Ошибка", JOptionPane.ERROR_MESSAGE);
            return;
        }
        if (connection.openPort(port)) {
            btnConnect.setText("Disconnect");
            btnConnect.setBackground(new Color(200, 0, 0));
            btnApply.setEnabled(true);
            btnStartScope.setEnabled(true);
            setStatus(false, "UNLOCKED");
        } else {
            JOptionPane.showMessageDialog(this, "Не удалось подключиться", "Ошибка", JOptionPane.ERROR_MESSAGE);
        }
    }

    private void disconnect() {
        connection.closePort();
        btnConnect.setText("Connect");
        btnConnect.setBackground(new Color(0, 150, 0));
        btnApply.setEnabled(false);
        btnStartScope.setEnabled(false);
        btnStopScope.setEnabled(false);
        setStatus(false, "DISCONNECTED");
        lblStatus.setBackground(Color.GRAY);
    }

    private boolean wasStreamingBeforeApply = false; 

    private void sendAll() {
        if (!connection.isOpen()) return;
        
        try {
            float freq = Float.parseFloat(txtFreq.getText().trim().replace(',', '.'));
            
            int powerCode = (Integer) cbPower.getSelectedItem();
            int cpCode = (Integer) cbCpCurrent.getSelectedItem();
            int ldMode = cbLdMode.getSelectedIndex(); 

            // Отправляем команду. Стрим НЕ прерывается.
            connection.sendControlPacket(freq, 
                    cpCode, 0, 0, 6, 200, 0xFF, powerCode, 0, 0, 0, 1, (short)0, ldMode
            );
            
            // Меняем статус визуально, но не трогаем кнопки Start/Stop
            setStatus(false, "SENDING...");
            lblStatus.setBackground(Color.ORANGE);

        } catch (Exception ex) {
            JOptionPane.showMessageDialog(this, "Ошибка ввода:\n" + ex.getMessage(), "Ошибка", JOptionPane.WARNING_MESSAGE);
        }
    }

    private void setStatus(boolean locked, String text) {
        lblStatus.setText(" " + text + " ");
        if (locked) lblStatus.setBackground(new Color(0, 180, 0));
        else        lblStatus.setBackground(new Color(220, 50, 50));
    }

    // --- Callbacks ---
    @Override
    public void onLogMessage(String message) {
        SwingUtilities.invokeLater(() -> {
            textLog.append(message + "\n");
            textLog.setCaretPosition(textLog.getDocument().getLength());
        });
    }

    @Override
    public void onLockStatusChanged(boolean isLocked) {
        SwingUtilities.invokeLater(() -> {
            if (lblStatus.getText().contains("SENDING")) return;
            setStatus(isLocked, isLocked ? "LOCKED" : "UNLOCKED");
        });
    }

    @Override
    public void onError(String error) {
        SwingUtilities.invokeLater(() -> textLog.append("[ERR] " + error + "\n"));
    }

    @Override
    public void onDataReceived(short[] samples) {
        SwingUtilities.invokeLater(() -> scopePanel.updateData(samples));
    }

    public static void main(String[] args) {
        try { UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName()); } catch (Exception ignored) {}
        SwingUtilities.invokeLater(() -> new Max2870Gui().setVisible(true));
    }
}