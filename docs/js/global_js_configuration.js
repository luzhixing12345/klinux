// 保存所有全局修改的配置


// 美化选择框
// - [ ] xxx
// - [x] aaa
var inputs = document.getElementsByTagName('input')
for (var i = 0; i < inputs.length; i++) {
    inputs[i].removeAttribute('disabled')
    inputs[i].onclick = function () {
        return false;
    }
}

var markdown_part = document.querySelector(".markdown-body");
// 在尾部添加一个 <div class="giscus"></div> 来加载 Giscus
var giscus = document.createElement('div');
giscus.setAttribute('class', 'giscus');
markdown_part.appendChild(giscus);

// 代码段可编辑, 可选中
var code_blocks = document.getElementsByTagName('pre');
for (var i = 0; i < code_blocks.length; i++) {
    code_blocks[i].setAttribute('contenteditable', 'true');
}

document.onkeydown = function (e) {
    // 对于左/右键被按下的情况, 切换至上一页下一页
    if (e.key === "ArrowLeft") {
        // console.log("左箭头键被按下");
        // 找到第一个 change-article 类的 button
        var button = document.querySelector(".change-article");
        if (button.getAttribute('url') !== '.') {
            window.location = button.getAttribute('url')
        }
    } else if (e.key === "ArrowRight") {
        // console.log("右箭头键被按下");
        // 找到最后一个 change-article 类的 button
        var button = document.querySelector(".change-article:last-child");
        if (button.getAttribute('url') !== '.') {
            window.location = button.getAttribute('url')
        }

    }
}