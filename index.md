# EMG signal processing platform

Платформа состоит из:
1. Тренировочные наборы данных и функции для работы с ними [.\data](https://github.com/estel1/emg_platform/blob/master/data/readme.md). 
2. Функции для предобработки, обнаружения и классификации сигналов EMG, реализующие различные алгоритмы.
3. Скрипты для **сравнения** различных алгоритмов.
4. Документация: описание алгоритов, ссылки на статьи, описание функций, отчеты с результатами.
5. [Аппаратная платформа](https://github.com/estel1/emg_platform/blob/master/hw_platform/readme.md).

## Начало работы
1. Установите [Git](https://git-scm.com/downloads).
2. Получите свою локальную копию платформы:
```bash
$ git clone https://github.com/estel1/emg_platform
```
3. Запустите пример [EMGClassific_SVMmetod_10mov.m](https://github.com/estel1/emg_platform/blob/master/source/matlab/EMGClassific_SVMmetod_10mov/EMGClassific_SVMmetod_10mov.m)
4. Пришлите мне свой username для включения в список collaborators (если у вас нет аккаунта - зарегистрируйтесь на [GitHub](https://github.com)

## Возможные темы 
### Разработка программных средств для анализа сигналов EMG с использованием моделей глубокого обучения.
1. Разработка программных средств для анализа сигналов EMG с использованием моделей глубокого обучения. 
  a. [PMC5013051](https://www.ncbi.nlm.nih.gov/pmc/articles/PMC5013051/).
  b. [Deep Learning for Electromyographic Hand Gesture Signal Classification Using Transfer Learning](https://arxiv.org/pdf/1801.07756.pdf).
2. Разработка программных средств для анализа сигналов EMG на основе адаптивных (рекалибруемых без повторного обучения) моделей. [28744189](https://www.ncbi.nlm.nih.gov/pubmed/28744189).
3. [V.T.GAIKWAD, 2M.M.SARDESHMUKH SIGN LANGUAGE RECOGNITION BASED ON ELECTROMYOGRAPHY (EMG) SIGNAL USING ARTIFICIAL NEURAL NETWORK (ANN). ](http://pep.ijieee.org.in/journal_pdf/11-66-140326324973-76.pdf)
4. [Machine Learning for Gesture Recognition with Electromyography](https://brage.bibsys.no/xmlui/bitstream/handle/11250/2459262/16780_FULLTEXT.pdf?sequence=1&isAllowed=y)
5. Исследование канала управления: подаем воздействие, например, сжимаем кисть, при этом контролируем силу сжатия с помощью прибора. Измеряем сигнал EMG. Строим прямую мат. модель импульс--сжатие с использованием Вольтерровского описания. (возможно учитывая усталость и т.п.). Цель - извлечение новых классификационных признаков из модели.

## Коммерческие продукты
1. [Myo Armband](https://www.myo.com/).

## Статьи
1. [Hand Gesture Recognition Using Machine Learning
and the Myo Armband](https://www.eurasip.org/Proceedings/Eusipco/Eusipco2017/papers/1570347665.pdf).
2. [Techniques of EMG signal analysis: detection, processing, classification and applications](https://www.ncbi.nlm.nih.gov/pmc/articles/PMC1455479/).

## Книги
1. SURFACE ELECTROMYOGRAPHY Physiology,Engineering,andApplications. 
2. Bita Mokhlesabadifarahani Vinit Kumar Gunjan EMG Signals Characterization in Three States of Contraction by Fuzzy Network and Feature Extraction.

## Ссылки
1. [ESP8266 Tutorial](https://tttapa.github.io/ESP8266/Chap01%20-%20ESP8266.html)
2. [A compendium of blog posts on op amp design topics
by Bruce Trump](http://www.ti.com/lit/ml/slyt701/slyt701.pdf).


You can use the [editor on GitHub](https://github.com/estel2/emg/edit/master/index.md) to maintain and preview the content for your website in Markdown files.

Whenever you commit to this repository, GitHub Pages will run [Jekyll](https://jekyllrb.com/) to rebuild the pages in your site, from the content in your Markdown files.

### Markdown

Markdown is a lightweight and easy-to-use syntax for styling your writing. It includes conventions for

```markdown
Syntax highlighted code block

# Header 1
## Header 2
### Header 3

- Bulleted
- List

1. Numbered
2. List

**Bold** and _Italic_ and `Code` text

[Link](url) and ![Image](src)
```

For more details see [GitHub Flavored Markdown](https://guides.github.com/features/mastering-markdown/).

### Jekyll Themes

Your Pages site will use the layout and styles from the Jekyll theme you have selected in your [repository settings](https://github.com/estel2/emg/settings). The name of this theme is saved in the Jekyll `_config.yml` configuration file.

### Support or Contact

Having trouble with Pages? Check out our [documentation](https://help.github.com/categories/github-pages-basics/) or [contact support](https://github.com/contact) and we’ll help you sort it out.
